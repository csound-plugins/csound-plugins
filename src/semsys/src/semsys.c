/*
    semsys.c:

    Copyright (C) 2026 Pasquale Mainolfi

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

/*
    SemSys is an experimental semantic synthesis framework for Csound
    designed to explore the creative potential of latent spaces in computer music,
    sound design, and algorithmic composition.
    The library provides opcodes for loading embedding models, generating sentence
    and token embeddings, building and querying semantic spaces, performing similarity
    search, nearest-neighbor retrieval, and semantic interpolation.
    By representing sounds, presets, gestures, processes, or textual descriptions as
    vectors in a shared latent space, SemSys enables composers and researchers to create
    semantic relationships between musical materials and navigate them using natural
    language or latent-space operations.
    SemSys is intended as both a practical toolkit and a research platform for investigating
    semantic representations in music technology.
*/

#include "csdl.h"
#include "csound.h"
#include "soundfile.h"
#include "sysdep.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <dlfcn.h>
#endif
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "semsys.h"
#include "../../common/_common.h"

/* allocate a csound array as a flat [numrows * numcols] buffer, then reshape it to a
   2D [numrows, numcols] array. used for the semspacequery neighbour matrix. */
static int32_t tabinit2d(CSOUND *csound, ARRAYDAT *arr, int numrows, int numcols, OPDS *ctx) {
    arr->dimensions = 1;
    tabinit_compat(csound, arr, numrows * numcols, ctx);
    arr->sizes = csound->ReAlloc(csound, arr->sizes, sizeof(int32_t) * 2);
    arr->dimensions = 2;
    arr->sizes[0] = numrows;
    arr->sizes[1] = numcols;
    return OK;
}

static inline int float_is_finite(float v) {
    union {
        float f;
        uint32_t u;
    } bits;
    bits.f = v;
    return (bits.u & 0x7F800000u) != 0x7F800000u;
}

static inline int double_is_finite(double v) {
    union {
        double d;
        uint64_t u;
    } bits;
    bits.d = v;
    return (bits.u & UINT64_C(0x7FF0000000000000)) != UINT64_C(0x7FF0000000000000);
}

static inline int vec_is_finite(const float *vec, uint32_t dim) {
    for (uint32_t i = 0; i < dim; i++) {
        if (!float_is_finite(vec[i])) {
            return 0;
        }
    }
    return 1;
}

static inline int normalize(float *vec, uint32_t dim) {
    double norm = 0.0;
    for (uint32_t i = 0; i < dim; i++) {
        if (!float_is_finite(vec[i])) {
            return NOTOK;
        }
        norm += (double) vec[i] * (double) vec[i];
    }

    norm = sqrt(norm);
    if (!double_is_finite(norm) || norm <= 0.0) {
        return NOTOK;
    }

    for (uint32_t i = 0; i < dim; i++) {
        vec[i] = (float) ((double) vec[i] / norm);
        if (!float_is_finite(vec[i])) {
            return NOTOK;
        }
    }
    return OK;
}

static inline float dot(float *vec_a, float *vec_b, uint32_t dim) {
    double d = 0.0;
    for (uint32_t i = 0; i < dim; i++) {
        d += (double)(vec_a[i] * vec_b[i]);
    }
    return (float) d;
}

static const char *skip_leading_query_junk(const char *s) {
    while (s != NULL && *s != '\0') {
        unsigned char c = (unsigned char) *s;
        if (isspace(c) || c == '-' || c == '"' || c == '\'' || c == ':' || c == ';') {
            s++;
            continue;
        }
        break;
    }
    return (s != NULL) ? s : "";
}

/* read one line (up to '\n' or EOF) from `fp`. returns a malloc'd line (caller frees),
   or NULL at EOF with nothing read (an empty line before EOF still returns ""). */
static char *read_line_from_file(FILE *fp) {
    size_t cap = 256;
    size_t len = 0;
    char *buf = malloc(cap);

    if (buf == NULL) return NULL;

    int c;
    while ((c = fgetc(fp)) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (tmp == NULL) {
                free(buf);
                return NULL;
            }
            buf = tmp;
        }

        if (c == '\n')
            break;

        buf[len++] = (char) c;
    }

    if (c == EOF && len == 0) {
        free(buf);
        return NULL;
    }

    buf[len] = '\0';

    if (len > 0 && buf[len - 1] == '\r')
        buf[len - 1] = '\0';

    return buf;
}

/* accumulate consecutive non-empty lines into one paragraph (a blank line separates
   paragraphs). returns a malloc'd paragraph (caller frees), or NULL at end of file. */
static char *read_paragraph_from_file(FILE *fp) {
    char *acc = NULL;
    size_t acc_len = 0;
    while (1) {
        char *line = read_line_from_file(fp);
        if (line == NULL) return acc;
        size_t llen = strlen(line);
        if (llen == 0) {
            free(line);
            if (acc == NULL) continue;
            return acc;
        }

        char *tmp = realloc(acc, acc_len + llen + 2);
        if (tmp == NULL) {
            free(line);
            free(acc);
            return NULL;
        }
        acc = tmp;
        if (acc_len > 0) acc[acc_len++] = ' ';
        memcpy(acc + acc_len, line, llen);
        acc_len += llen;
        acc[acc_len] = '\0';
        free(line);
    }
}

static size_t chunk_max_chars(uint32_t maxlen_seq) {
    size_t max_chars = (size_t) maxlen_seq * 32u;
    return max_chars > 256u ? max_chars : 256u;
}

static size_t utf8_boundary_before(const char *s, size_t start, size_t end, size_t limit) {
    if (end >= limit) {
        return end;
    }
    while (end > start && (((unsigned char) s[end]) & 0xC0) == 0x80) {
        end--;
    }
    return end > start ? end : limit;
}

static void heapfy_up(Score *mheap, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (mheap[idx].score >= mheap[parent].score) break;
        Score tmp = mheap[idx];
        mheap[idx] = mheap[parent];
        mheap[parent] = tmp;
        idx = parent;
    }
}

static void heapfy_down(Score *mheap, int index, int size) {
    while (true) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int smallest = index;

        if (left < size && mheap[left].score < mheap[smallest].score) smallest = left;
        if (right < size && mheap[right].score < mheap[smallest].score) smallest = right;
        if (smallest == index) break;

        Score tmp = mheap[index];
        mheap[index] = mheap[smallest];
        mheap[smallest] = tmp;
        index = smallest;
    }
}

static int compare(const void *a, const void *b) {
    float d = ((Score *)b)->score - ((Score *)a)->score;
    return (d > 0) - (d < 0);
}

// CORE

/* release every handle owned by ctx, null-safe and idempotent */
static void release_ctx(CSOUND *csound, SEMSYS *ctx) {
    if (ctx == NULL) {
        return;
    }
    if (ctx->api != NULL) {
        if (ctx->emb_session != NULL) {
            ctx->api->ReleaseSession(ctx->emb_session);
            ctx->emb_session = NULL;
        }
        if (ctx->emb_session_options != NULL) {
            ctx->api->ReleaseSessionOptions(ctx->emb_session_options);
            ctx->emb_session_options = NULL;
        }
        if (ctx->env != NULL) {
            ctx->api->ReleaseEnv(ctx->env);
            ctx->env = NULL;
        }
    }
    csound->Free(csound, ctx);
}

static int32_t sem_deinit(CSOUND *csound, void *vp) {
    SEM_INIT *p = (SEM_INIT *) vp;
    SEMSYS *ctx = (SEMSYS *)(uintptr_t) *p->handle;
    release_ctx(csound, ctx);
    *p->handle = FL(0.0);
    return OK;
}

#ifdef _WIN32
/* UTF-8 -> wide string (ORTCHAR_T is wchar_t on Windows). malloc'd, caller frees. */
static wchar_t *utf8_to_wide(const char *s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (n <= 0) return NULL;
    wchar_t *w = (wchar_t *) malloc((size_t) n * sizeof(wchar_t));
    if (w == NULL) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n);
    return w;
}
#endif

/* CreateSession from a UTF-8 path. The ORT C API takes ORTCHAR_T* (wchar_t on
   Windows, char elsewhere); this converts as needed so callers stay portable. */
static OrtStatus *create_session_utf8(const OrtApi *api, OrtEnv *env, const char *path, OrtSessionOptions *opts, OrtSession **out) {
#ifdef _WIN32
    wchar_t *wpath = utf8_to_wide(path);
    if (wpath == NULL) {
        return api->CreateStatus(ORT_FAIL, "[semload] could not convert model path");
    }
    OrtStatus *st = api->CreateSession(env, wpath, opts, out);
    free(wpath);
    return st;
#else
    return api->CreateSession(env, path, opts, out);
#endif
}

/* directory containing this plugin binary (the .dylib/.so/.dll), so we can find
   sibling bundled libraries. returns 0 on success. */
static int plugin_dir(char *buf, size_t size) {
#ifdef _WIN32
    HMODULE hm = NULL;
    if (!GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR) &plugin_dir,
        &hm)
    ) {
        return -1;
    }
    if (GetModuleFileNameA(hm, buf, (DWORD) size) == 0) return -1;
    char *slash = strrchr(buf, '\\');
    if (slash != NULL) *slash = '\0';
    return 0;
#else
    Dl_info info;
    if (dladdr((void *) &plugin_dir, &info) == 0 || info.dli_fname == NULL) return -1;
    snprintf(buf, size, "%s", info.dli_fname);
    char *slash = strrchr(buf, '/');
    if (slash != NULL) *slash = '\0';
    return 0;
#endif
}

/* the end-to-end graph bakes preprocessing in as onnxruntime-extensions custom ops
   (tokenizer for the embedding model, audio decoder/feature extractor for STT), so the
   extensions shared library must be registered when the session is created. resolve it
   in order: 1. env SEMSYS_ORT_EXTENSIONS (full path), 2. bundled next to the plugin,
   3. next to the model files in model_dir. */
static const char *resolve_extensions_path(const char *model_dir, char *buf, size_t bufsize) {
    const char *env = getenv("SEMSYS_ORT_EXTENSIONS");
    if (env != NULL && env[0] != '\0') {
        return env;
    }
#ifdef _WIN32
    const char *libname = "ortextensions.dll";
#elif defined(__APPLE__)
    const char *libname = "libortextensions.dylib";
#else
    const char *libname = "libortextensions.so";
#endif

    /* bundled next to the plugin */
    char dir[1024];
    if (plugin_dir(dir, sizeof(dir)) == 0) {
        snprintf(buf, bufsize, "%s/%s", dir, libname);
        FILE *t = fopen(buf, "rb");
        if (t != NULL) { fclose(t); return buf; }
    }

    /* fallback: next to the model files */
    size_t dl = strlen(model_dir);
    const char *sep = (dl > 0 && (model_dir[dl - 1] == '/' || model_dir[dl - 1] == '\\')) ? "" : "/";
    snprintf(buf, bufsize, "%s%s%s", model_dir, sep, libname);
    return buf;
}

int sem_init(CSOUND *csound, SEM_INIT *p) {
    if (p->model_dir == NULL || p->model_dir->data == NULL) {
        return csound->InitError(csound, "[semload] Missing model dir");
    }

    SEMSYS *ctx = (SEMSYS *) csound->Calloc(csound, sizeof(SEMSYS));
    if (ctx == NULL) {
        return csound->InitError(csound, "[semload] Could not allocate context");
    }

    int ret = OK;

    ctx->maxlen_seq = (int32_t) *p->maxlen_seq;

    /* publish handle before anything can fail; cleanup runs via the API7 OENTRY deinit slot */
    *p->handle = CTX_TO_FLT(ctx);

    const char *mdir = (const char *) p->model_dir->data;
    size_t mdlen = strlen(mdir);
    const char *sep = (mdlen > 0 && (mdir[mdlen - 1] == '/' || mdir[mdlen - 1] == '\\')) ? "" : "/";
    snprintf(ctx->model_path, sizeof(ctx->model_path), "%s%smodel.onnx", mdir, sep);

    // check if model files exists and are valid
    char model_data_path[1024];
    snprintf(model_data_path, sizeof(model_data_path), "%s%smodel.onnx.data", mdir, sep);
    FILE *fmcheck = fopen(ctx->model_path, "rb");
    FILE *fdcheck = fopen(model_data_path, "rb");
    if (fmcheck == NULL || fdcheck == NULL) {
        if (fmcheck != NULL) fclose(fmcheck);
        if (fdcheck != NULL) fclose(fdcheck);
        ret = csound->InitError(csound, "[semload] Missing model file: need model.onnx and model.onnx.data in %s", mdir);
        goto fail;
    }

    fclose(fmcheck);
    fclose(fdcheck);
    // ---

    ctx->api = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (ctx->api == NULL) {
        return csound->InitError(csound, "[semload] Could not get onnxruntime api");
    }

    ONNX_CHECK_INIT(ctx->api, ctx->api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "SemSys", &ctx->env));
    ONNX_CHECK_INIT(ctx->api, ctx->api->CreateSessionOptions(&ctx->emb_session_options));
    /* the E2E embedding graph tokenizes internally with onnxruntime-extensions
       contrib ops (ai.onnx.contrib) -> register the extensions library */
    char extbuf[1024];
    const char *extpath = resolve_extensions_path(mdir, extbuf, sizeof(extbuf));
    void *exthandle = NULL;
    ONNX_CHECK_INIT(ctx->api, ctx->api->RegisterCustomOpsLibrary(ctx->emb_session_options, extpath, &exthandle));
    ONNX_CHECK_INIT(ctx->api, create_session_utf8(ctx->api, ctx->env, ctx->model_path, ctx->emb_session_options, &ctx->emb_session));

    OrtTypeInfo *type_info = NULL;
    ONNX_CHECK_INIT(ctx->api, ctx->api->SessionGetOutputTypeInfo(ctx->emb_session, 0, &type_info));

    const OrtTensorTypeAndShapeInfo *shape_info = NULL;
    OrtStatus *status = ctx->api->CastTypeInfoToTensorInfo(type_info, &shape_info);
    if (status != NULL) {
        const char *msg = ctx->api->GetErrorMessage(status);
        int ret = csound->InitError(csound, "[semload] onnxruntime error: %s", msg);
        ctx->api->ReleaseStatus(status);
        ctx->api->ReleaseTypeInfo(type_info);
        return ret;
    }

    int64_t dims[8] = { 0 };
    size_t num_dims = 0;
    status = ctx->api->GetDimensionsCount(shape_info, &num_dims);
    if (status == NULL) {
        status = ctx->api->GetDimensions(shape_info, dims, num_dims < 8 ? num_dims : 8);
    }
    ctx->api->ReleaseTypeInfo(type_info);
    if (status != NULL) {
        const char *msg = ctx->api->GetErrorMessage(status);
        int ret = csound->InitError(csound, "[semload] onnxruntime error: %s", msg);
        ctx->api->ReleaseStatus(status);
        return ret;
    }

    if (num_dims == 0) {
        return csound->InitError(csound, "[semload] Model output has no dimensions");
    }

    int64_t last = dims[(num_dims < 8 ? num_dims : 8) - 1];
    if (last <= 0) {
        return csound->InitError(csound, "[semload] Model has dynamic/invalid embedding dim (%lld)", (long long) last);
    }
    ctx->ldim = (int32_t) last;

    return OK;

fail:
    sem_deinit(csound, p);
    return ret;
}

int sem_dim(CSOUND *csound, SEM_DIM *p) {
    SEMSYS *ctx = FLT_TO_CTX(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semdim] Not valid handle");
    }

    *p->out_dim = (MYFLT) ctx->ldim;
    return OK;
}

/* run the E2E embedding model: a raw text string -> pooled sentence embedding
   [ldim]. the graph tokenizes internally (onnxruntime-extensions contrib ops),
   so there is no separate tokenizer session and no per-token output. */
static int embed_e2e(SEMSYS *ctx, const char *text, MYFLT *pool_embed) {
    int64_t input_shape[] = { 1 };
    OrtAllocator *alloc = NULL;
    OrtValue *input_tensor = NULL;
    OrtValue *outs[1] = { NULL };
    int ret = 1;

    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetAllocatorWithDefaultOptions(&alloc));
    ONNX_CHECK_GOTO(ctx->api, ctx->api->CreateTensorAsOrtValue(alloc, input_shape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING, &input_tensor));
    ONNX_CHECK_GOTO(ctx->api, ctx->api->FillStringTensor(input_tensor, &text, 1));

    const char *in_names[] = { "text" };
    const OrtValue *ins[] = { input_tensor };
    const char *out_names[] = { "embedding" };

    ONNX_CHECK_GOTO(ctx->api, ctx->api->Run(ctx->emb_session, NULL, in_names, ins, 1, out_names, 1, outs));

    /* pooled embedding float32 [1, ldim] */
    float *emb = NULL;
    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetTensorMutableData(outs[0], (void **)&emb));
    for (uint32_t d = 0; d < ctx->ldim; d++) pool_embed[d] = (MYFLT) emb[d];

    ret = 0;

fail:
    if (outs[0] != NULL) ctx->api->ReleaseValue(outs[0]);
    if (input_tensor != NULL) ctx->api->ReleaseValue(input_tensor);
    return ret;
}

/* embed one sentence into the pooled vector `pmb` (ldim).
   returns 1 on success, 0 for empty text (caller skips), NOTOK on error. */
static int embed_sentence(MYFLT *pmb, SEMSYS *ctx, const char *sentence) {
    if (sentence == NULL || sentence[0] == '\0') return 0;
    if (embed_e2e(ctx, sentence, pmb) != 0) return NOTOK;
    return 1;
}

/* defined below; reused to chunk + embed text into a growing per-chunk buffer */
static int chunk_and_embed(const char *paragraph, FILE *dest_file, float **dest_vec, SEMSYS *ctx, MYFLT *pmb, float *pool, uint64_t *count);

static int sem_embed_init_helper(CSOUND *csound, SEM_EMBED *p) {
    SEMSYS *ctx = FLT_TO_CTX(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semembed] Not valid handle");
    }

    p->ctx = ctx;

    /* k-rate output is a single [ldim] vector: long text is chunked and the chunk
       embeddings are mean-pooled into it (2D per-chunk output can't grow rows at perf) */
    tabinit_compat(csound, p->pool_embed, ctx->ldim, &(p->h));

    return OK;
}

int sem_embed_init(CSOUND *csound, SEM_EMBED *p) {
    int err = sem_embed_init_helper(csound, p);
    if (err != OK) { return err; }

    /* empty cache forces embedding on the first perf pass */
    csound->AuxAlloc(csound, 1, &p->last_text);
    ((char *) p->last_text.auxp)[0] = '\0';

    return OK;
}

/* embed `text` into the k-rate output vector [ldim]: long text is chunked
   (chunk_and_embed) and the per-chunk embeddings are mean-pooled into one centroid
   vector, so the whole text contributes instead of being truncated. called only on text
   change from perf, so the per-change malloc cost is negligible. */
static int sem_embed_ik(CSOUND *csound, SEM_EMBED *p, const char *text) {
    (void) csound;
    SEMSYS *ctx = p->ctx;

    MYFLT *pmb = (MYFLT *) malloc(sizeof(MYFLT) * ctx->ldim);
    float *pool = (float *) malloc(sizeof(float) * ctx->ldim);
    if (pmb == NULL || pool == NULL) {
        free(pmb);
        free(pool);
        return NOTOK;
    }

    float *vecs = NULL;
    uint64_t count = 0;
    int rc = chunk_and_embed(text, NULL, &vecs, ctx, pmb, pool, &count);
    free(pmb);
    free(pool);
    if (rc != OK) {
        free(vecs);
        return NOTOK;
    }

    MYFLT *out = (MYFLT *) p->pool_embed->data;
    if (count == 0) {
        for (uint32_t j = 0; j < ctx->ldim; j++) {
            out[j] = FL(0.0);   /* empty text -> zero vector */
        }
        free(vecs);
        return OK;
    }

    /* mean-pool the per-chunk (normalized) embeddings into the single output vector */
    for (uint32_t j = 0; j < ctx->ldim; j++) {
        double acc = 0.0;
        for (uint64_t c = 0; c < count; c++) {
            acc += (double) vecs[c * ctx->ldim + j];
        }
        out[j] = (MYFLT) (acc / (double) count);
    }
    free(vecs);

    return OK;
}

int sem_embed_perf(CSOUND *csound, SEM_EMBED *p) {
    *p->gate = FL(0.0);

    const char *text = (p->text != NULL && p->text->data != NULL) ? p->text->data : "";

    /* self-gate: skip the model when text is unchanged */
    if (strcmp(text, (char *) p->last_text.auxp) == 0) { return OK; }

    /* cache the new text (grow buffer only on change - rare) */
    size_t tlen = strlen(text) + 1;
    if (tlen > (size_t) p->last_text.size) {
        csound->AuxAlloc(csound, tlen, &p->last_text);
    }

    strcpy((char *) p->last_text.auxp, text);

    int err = sem_embed_ik(csound, p, text);
    if (err == NOTOK) {
        return csound->PerfError(csound, &(p->h), "[semembed] Embedding process error");
    }

    *p->gate = FL(1.0);

    return OK;
}

/* i-rate form: embeds at init and returns a 2D array [nchunks, ldim]. long text is
   chunked with the same token-window logic as semspacebuild (reused via chunk_and_embed),
   one row per chunk. short text yields a single row. */
int sem_embed_i(CSOUND *csound, SEM_EMBED_TEXT_I *p) {
    SEMSYS *ctx = FLT_TO_CTX(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semembed] Not valid handle");
    }
    p->ctx = ctx;

    const char *text = (p->text != NULL && p->text->data != NULL) ? p->text->data : "";

    /* scratch buffers for the embed -> normalize -> append pipeline */
    MYFLT *pmb = (MYFLT *) malloc(sizeof(MYFLT) * ctx->ldim);
    float *pool = (float *) malloc(sizeof(float) * ctx->ldim);
    if (pmb == NULL || pool == NULL) {
        free(pmb);
        free(pool);
        return csound->InitError(csound, "[semembed] Out of memory");
    }

    /* treat the whole input as one paragraph; chunk_and_embed splits it into token
       windows and appends one ldim-row per chunk to the growing buffer */
    float *vecs = NULL;
    uint64_t count = 0;
    int rc = chunk_and_embed(text, NULL, &vecs, ctx, pmb, pool, &count);
    free(pmb);
    free(pool);

    if (rc != OK) {
        free(vecs);
        return csound->InitError(csound, "[semembed] Embedding process error");
    }

    /* [count, ldim] output; empty text -> zero rows */
    tabinit2d(csound, p->pool_embed, (int) count, (int) ctx->ldim, &(p->h));
    MYFLT *out = p->pool_embed->data;
    for (uint64_t i = 0; i < count * ctx->ldim; i++) {
        out[i] = (MYFLT) vecs[i];
    }
    free(vecs);

    return OK;
}

int sem_embed_i_file(CSOUND *csound, SEM_EMBED_FILE_I *p) {
    SEMSYS *ctx = FLT_TO_CTX(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semembed] Not valid handle");
    }
    p->ctx = ctx;

    FILE *fp = fopen(p->file_path->data, "rb");
    if (fp == NULL) {
        return csound->InitError(csound, "[semembed] Cannot open file: %s", p->file_path->data);
    }

    fseek(fp, 0, SEEK_END);
    long fsz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fsz <= 0) {
        fclose(fp);
        return csound->InitError(csound, "[semembed] Empty file");
    }

    /* +1 for the NUL terminator: chunk_and_embed uses strlen() */
    char *text = (char *) malloc((size_t) fsz + 1);
    if (text == NULL) {
        fclose(fp);
        return csound->InitError(csound, "[semembed] Out of memory reading file");
    }

    size_t got = fread(text, 1, (size_t) fsz, fp);
    fclose(fp);
    if (got != (size_t) fsz) {
        free(text);
        return csound->InitError(csound, "[semembed] Short read on file");
    }
    text[fsz] = '\0';

    /* scratch buffers for the embed -> normalize -> append pipeline */
    MYFLT *pmb = (MYFLT *) malloc(sizeof(MYFLT) * ctx->ldim);
    float *pool = (float *) malloc(sizeof(float) * ctx->ldim);
    if (pmb == NULL || pool == NULL) {
        free(pmb);
        free(pool);
        free(text);
        return csound->InitError(csound, "[semembed] Out of memory");
    }

    /* treat the whole file as one paragraph; chunk_and_embed splits it into token
       windows and appends one ldim-row per chunk to the growing buffer */
    float *vecs = NULL;
    uint64_t count = 0;
    int rc = chunk_and_embed(text, NULL, &vecs, ctx, pmb, pool, &count);
    free(pmb);
    free(pool);
    free(text);
    if (rc != OK) {
        free(vecs);
        return csound->InitError(csound, "[semembed] Embedding process error");
    }

    /* [count, ldim] output; empty text -> zero rows */
    tabinit2d(csound, p->pool_embed, (int) count, (int) ctx->ldim, &(p->h));
    MYFLT *out = p->pool_embed->data;
    for (uint64_t i = 0; i < count * ctx->ldim; i++) {
        out[i] = (MYFLT) vecs[i];
    }
    free(vecs);

    return OK;
}

static int32_t sem_space_deinit(CSOUND *csound, void *vp) {
    SEM_SPACE_INIT *p = (SEM_SPACE_INIT *) vp;
    SEMSYS_SPACE *spc = (SEMSYS_SPACE *)(uintptr_t) *p->s_handle;
    if (spc != NULL) {
        csound->Free(csound, spc->vectors);
        csound->Free(csound, spc);
    }
    *p->s_handle = FL(0.0);
    return OK;
}

static void sem_space_build_deinit(CSOUND *csound, SEM_SPACE_BUILD *p, FILE *fptr, FILE *source) {
    if (fptr != NULL) fclose(fptr);
    if (source != NULL) fclose(source);
    csound->Free(csound, p->pmb);
    free(p->pool);
}

/* embed `text` and append the normalized vector to a sink: either a file (`dest_file`)
   or a growing in-RAM buffer (`*dest_vec`, realloc'd to hold `*count`+1 rows of ldim
   floats; the caller owns and frees it). `*count` is the number of rows written so far
   and is bumped on success. an empty/skipped text writes nothing and returns OK. */
static int cat_embedding(FILE *dest_file, float **dest_vec, SEMSYS *ctx, MYFLT *pmb, float *pool, const char *text, uint64_t *count) {
    int err = embed_sentence(pmb, ctx, text);
    if (err == NOTOK) {
        return NOTOK;
    }
    if (err == 0) {
        return OK;
    }

    for (uint32_t i = 0; i < ctx->ldim; i++) {
        pool[i] = (float) pmb[i];
    }
    if (normalize(pool, ctx->ldim) != OK) {
        return NOTOK;
    }
    if (dest_file != NULL) {
        if (fwrite(pool, sizeof(float), ctx->ldim, dest_file) != ctx->ldim) {
            return NOTOK;
        }
    } else if (dest_vec != NULL) {
        uint64_t c = *count;                       /* rows already stored */
        float *grown = (float *) realloc(*dest_vec, sizeof(float) * (c + 1) * ctx->ldim);
        if (grown == NULL) {
            return NOTOK;
        }
        *dest_vec = grown;
        memcpy(grown + c * ctx->ldim, pool, sizeof(float) * ctx->ldim);
    }

    (*count)++;
    return OK;
}

/* chunk one paragraph string into overlapping token windows (sub-splitting any window
   whose byte length exceeds the model char limit, at a UTF-8 boundary) and embed each
   into the sink (dest_file or *dest_vec). does NOT take ownership of `paragraph`.
   `*count` is advanced per embedded window. returns OK, or NOTOK on error. */
static int chunk_and_embed(const char *paragraph, FILE *dest_file, float **dest_vec,
                           SEMSYS *ctx, MYFLT *pmb, float *pool, uint64_t *count) {
    size_t plen = strlen(paragraph);

    int wsize = TOKWIN(ctx->maxlen_seq);
    int stride = TOKSTRIDE(wsize);

    size_t n_words = 0;
    size_t cap = 0;
    size_t *word_start = NULL;
    int in_word = 0;

    for (size_t i = 0; i < plen; i++) {
        if (!(isspace((unsigned char) paragraph[i]))) {
            if (!in_word) {
                if (n_words == cap) {
                    cap = cap ? cap * 2 : 64;
                    size_t *tmp = realloc(word_start, cap * sizeof(size_t));
                    if (tmp == NULL) {
                        free(word_start);
                        return NOTOK;
                    }
                    word_start = tmp;
                }
                word_start[n_words++] = i;
            }
            in_word = 1;
        } else {
            in_word = 0;
        }
    }

    size_t max_chars = chunk_max_chars(ctx->maxlen_seq);
    char *window = malloc(plen + 1);
    if (window == NULL) {
        free(word_start);
        return NOTOK;
    }

    for (size_t i = 0; i < n_words; i += stride) {
        size_t k = (i + wsize) < n_words ? (i + wsize) : n_words;
        size_t start = word_start[i];
        size_t end = (k < n_words) ? word_start[k] : plen;
        size_t wlen = end - start;
        if (wlen <= max_chars) {
            memcpy(window, paragraph + start, wlen);
            window[wlen] = '\0';
            if (cat_embedding(dest_file, dest_vec, ctx, pmb, pool, window, count) != OK) {
                free(window);
                free(word_start);
                return NOTOK;
            }
        } else {
            size_t pos = start;
            while (pos < end) {
                size_t chunk_end = pos + max_chars;
                if (chunk_end > end) {
                    chunk_end = end;
                } else {
                    chunk_end = utf8_boundary_before(paragraph, pos, chunk_end, end);
                }
                size_t clen = chunk_end - pos;
                memcpy(window, paragraph + pos, clen);
                window[clen] = '\0';
                if (cat_embedding(dest_file, dest_vec, ctx, pmb, pool, window, count) != OK) {
                    free(window);
                    free(word_start);
                    return NOTOK;
                }
                pos = chunk_end;
            }
        }

        if (k == n_words) break;
    }

    free(word_start);
    free(window);
    return OK;
}

/* read `source` file paragraph-by-paragraph and embed each into the sink. returns the
   number of vectors written, or (uint64_t) NOTOK on error (callers test (int) ret == NOTOK).
   in-RAM text is chunked directly via chunk_and_embed (see semembed i-rate). */
static uint64_t sem_space_create_helper(FILE *source, FILE *dest_file, float **dest_vec, SEMSYS *ctx, MYFLT *pmb, float *pool) {
    uint64_t count = 0;
    char *paragraph;
    while ((paragraph = read_paragraph_from_file(source)) != NULL) {
        int rc = chunk_and_embed(paragraph, dest_file, dest_vec, ctx, pmb, pool, &count);
        free(paragraph);
        if (rc != OK) {
            return (uint64_t) NOTOK;
        }
    }

    return count;
}

/* duplicate a C string with malloc (portable strdup) */
static char *dup_str(const char *s) {
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (d != NULL) memcpy(d, s, n);
    return d;
}

static void free_str_list(char **list, size_t n) {
    if (list == NULL) return;
    for (size_t i = 0; i < n; i++) free(list[i]);
    free(list);
}

/* list full paths of files with extension `ext` (e.g. ".txt") directly under dirpath.
   returns 0 on success (*out / *out_n filled; free with free_str_list),
   -1 if the directory cannot be opened. an empty dir yields 0 with *out_n == 0. */
static int list_files_ext(const char *dirpath, const char *ext, char ***out, size_t *out_n) {
    char **names = NULL;
    size_t n = 0, cap = 0;
    char full[1024];
    size_t dl = strlen(dirpath);
    int has_sep = dl > 0 && (dirpath[dl - 1] == '/' || dirpath[dl - 1] == '\\');

#ifdef _WIN32
    char pattern[1024];
    snprintf(pattern, sizeof(pattern), "%s%s*%s", dirpath, has_sep ? "" : "\\", ext);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) { *out = NULL; *out_n = 0; return 0; }
        return -1;
    }
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        snprintf(full, sizeof(full), "%s%s%s", dirpath, has_sep ? "" : "\\", fd.cFileName);
        if (n == cap) { cap = cap ? cap * 2 : 16; names = realloc(names, cap * sizeof(char *)); }
        names[n++] = dup_str(full);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR *dir = opendir(dirpath);
    if (dir == NULL) return -1;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        const char *e = strrchr(entry->d_name, '.');
        if (e == NULL || strcmp(e, ext) != 0) continue;
        snprintf(full, sizeof(full), "%s%s%s", dirpath, has_sep ? "" : "/", entry->d_name);
        struct stat st;
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue; // skip dirs / non-regular
        if (n == cap) { cap = cap ? cap * 2 : 16; names = realloc(names, cap * sizeof(char *)); }
        names[n++] = dup_str(full);
    }
    closedir(dir);
#endif

    *out = names;
    *out_n = n;
    return 0;
}

/* portable directory test */
static int is_dir(const char *path) {
#ifdef _WIN32
    DWORD a = GetFileAttributesA(path);
    return (a != INVALID_FILE_ATTRIBUTES) && (a & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
#endif
}

/* append every vector from a .espc file into spc (growing the RAM buffer).
   validates the magic tags and that the file ldim matches the space.
   returns OK, or NOTOK on any error. */
static int load_espc_into(CSOUND *csound, SEMSYS_SPACE *spc, const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) return NOTOK;

    CACHE_HEADER ch;
    if (fread(&ch, sizeof(CACHE_HEADER), 1, f) != 1) { fclose(f); return NOTOK; }
    if (ch.head_tag != CACHE_HEADER_TAG || ch.data_tag != CACHE_DATA_TAG) { fclose(f); return NOTOK; }
    if (ch.ldim != spc->ldim) { fclose(f); return NOTOK; }

    uint64_t new_count = spc->count + ch.count;
    if (new_count > spc->capacity) {
        float *tmp = (float *) csound->ReAlloc(csound, spc->vectors, sizeof(float) * new_count * spc->ldim);
        if (tmp == NULL) { fclose(f); return NOTOK; }
        spc->vectors = tmp;
        spc->capacity = new_count;
    }

    uint64_t old_count = spc->count;
    float *base = spc->vectors + old_count * spc->ldim;
    size_t got = fread(base, sizeof(float) * spc->ldim, ch.count, f);
    fclose(f);
    if (got != ch.count) return NOTOK;

    uint64_t valid_count = 0;
    for (uint64_t i = 0; i < ch.count; i++) {
        float *vec = base + i * spc->ldim;
        if (!vec_is_finite(vec, spc->ldim) || normalize(vec, spc->ldim) != OK) {
            continue;
        }
        if (valid_count != i) {
            memmove(base + valid_count * spc->ldim, vec, sizeof(float) * spc->ldim);
        }
        valid_count++;
    }

    spc->count = old_count + valid_count;
    return OK;
}

// from dir of files (.txt) to .espc
// build a .espc from a text file, or from a directory of .txt files (auto-detected)
int sem_space_build(CSOUND *csound, SEM_SPACE_BUILD *p) {
    SEMSYS *ctx = FLT_TO_CTX(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semspacebuild] Null semsys context");
    }

    char *src = (char *) p->source->data;

    /* a directory expands to its .txt files; a plain path is a single file */
    char **files = NULL;
    size_t nfiles = 0;
    char *single_arr[1];
    char **list;

    if (is_dir(src)) {
        if (list_files_ext(src, ".txt", &files, &nfiles) != 0) {
            return csound->InitError(csound, "[semspacebuild] Cannot open directory");
        }
        list = files;
    } else {
        single_arr[0] = src;
        list = single_arr;
        nfiles = 1;
    }

    FILE *fptr = fopen((char *) p->file_name->data, "wb+");
    if (fptr == NULL) {
        free_str_list(files, nfiles);
        return csound->InitError(csound, "[semspacebuild] Cannot create dest file");
    }

    CACHE_HEADER ch;
    ch.head_tag = CACHE_HEADER_TAG;
    ch.ldim = ctx->ldim;
    ch.count = 0;
    ch.data_tag = CACHE_DATA_TAG;
    fwrite(&ch, sizeof(CACHE_HEADER), 1, fptr);

    p->pmb = csound->Calloc(csound, sizeof(MYFLT) * ctx->ldim);
    p->pool = (float *) calloc(ctx->ldim, sizeof(float));

    int failed = 0; /* 0 = ok, 1 = open error, 2 = embedding error */
    char failpath[1024] = { 0 };
    for (size_t i = 0; i < nfiles; i++) {
        FILE *cf = fopen(list[i], "rb");
        if (cf == NULL) {
            failed = 1;
            snprintf(failpath, sizeof(failpath), "%s", list[i]);
            break;
        }
        uint64_t _count = sem_space_create_helper(cf, fptr, NULL, ctx, p->pmb, p->pool);
        fclose(cf);
        if ((int) _count == NOTOK) {
            failed = 2;
            snprintf(failpath, sizeof(failpath), "%s", list[i]);
            break;
        }
        ch.count += _count;
    }

    free_str_list(files, nfiles);

    if (failed) {
        sem_space_build_deinit(csound, p, fptr, NULL);
        if (failed == 1) {
            return csound->InitError(csound, "[semspacebuild] Cannot open source file: %s", failpath);
        }
        return csound->InitError(csound, "[semspacebuild] Embedding error in: %s", failpath);
    }

    fseek(fptr, offsetof(CACHE_HEADER, count), SEEK_SET);
    fwrite(&ch.count, sizeof(ch.count), 1, fptr);
    fflush(fptr);

    sem_space_build_deinit(csound, p, fptr, NULL);

    return OK;
}

/* allocate + init a SEMSYS_SPACE bound to the model handle. returns NULL on error. */
static SEMSYS_SPACE *sem_space_alloc(CSOUND *csound, SEMSYS *ctx) {
    SEMSYS_SPACE *spc = (SEMSYS_SPACE *) csound->Calloc(csound, sizeof(SEMSYS_SPACE));
    if (spc == NULL) return NULL;
    spc->ctx = ctx;
    spc->ldim = ctx->ldim;
    spc->count = 0;
    spc->capacity = 0;
    spc->vectors = NULL;
    return spc;
}

// forms: semspace ihandle [, Sfile_or_dir]
int sem_space_init(CSOUND *csound, SEM_SPACE_INIT *p) {
    SEMSYS *ctx = FLT_TO_CTX(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semspace] Null semsys context");
    }

    SEMSYS_SPACE *spc = sem_space_alloc(csound, ctx);
    if (spc == NULL) {
        return csound->InitError(csound, "[semspace] Null space context");
    }

    const char *path = (p->cache_file != NULL && p->cache_file->data != NULL) ? p->cache_file->data : NULL;

    if (path == NULL || path[0] == '\0') {
        /* empty RAM-only space */
        spc->capacity = INITIAL_VCAPACITY;
        spc->vectors = (float *) csound->Calloc(csound, sizeof(float) * spc->ldim * spc->capacity);
    } else if (is_dir(path)) {
        /* load every .espc in the directory */
        char **files = NULL;
        size_t nf = 0;
        if (list_files_ext(path, ".espc", &files, &nf) != 0) {
            csound->Free(csound, spc);
            return csound->InitError(csound, "[semspace] Cannot open directory");
        }
        for (size_t i = 0; i < nf; i++) {
            if (load_espc_into(csound, spc, files[i]) != OK) {
                free_str_list(files, nf);
                csound->Free(csound, spc->vectors);
                csound->Free(csound, spc);
                return csound->InitError(csound, "[semspace] Error loading .espc from directory (check format / ldim)");
            }
        }
        free_str_list(files, nf);
    } else {
        /* single .espc file */
        if (load_espc_into(csound, spc, path) != OK) {
            csound->Free(csound, spc->vectors);
            csound->Free(csound, spc);
            return csound->InitError(csound, "[semspace] Cannot load .espc file (check format / ldim)");
        }
    }

    *p->s_handle = SPC_TO_FLT(spc);
    return OK;
}

// form: semspace ihandle, Spaths[]  -> merge several .espc files
int sem_space_init_vs(CSOUND *csound, SEM_SPACE_INIT_VS *p) {
    SEMSYS *ctx = FLT_TO_CTX(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semspace] Null semsys context");
    }

    SEMSYS_SPACE *spc = sem_space_alloc(csound, ctx);
    if (spc == NULL) {
        return csound->InitError(csound, "[semspace] Null space context");
    }

    ARRAYDAT *arr = p->paths;
    int32_t npaths = (arr != NULL && arr->dimensions >= 1) ? arr->sizes[0] : 0;
    STRINGDAT *items = (arr != NULL) ? (STRINGDAT *) arr->data : NULL;

    for (int32_t i = 0; i < npaths; i++) {
        const char *path = items[i].data;
        if (path == NULL || path[0] == '\0') continue;
        if (load_espc_into(csound, spc, path) != OK) {
            csound->Free(csound, spc->vectors);
            csound->Free(csound, spc);
            return csound->InitError(csound, "[semspace] Error loading .espc from path array (check format / ldim)");
        }
    }

    *p->s_handle = SPC_TO_FLT(spc);
    return OK;
}

int sem_space_add_init(CSOUND *csound, SEM_SPACE_ADD *p) {
    SEMSYS_SPACE *spc = FLT_TO_SPC(p);
    if (spc == NULL) {
        return csound->InitError(csound, "[semspaceadd] Null space context");
    }

    p->ctx = spc->ctx;
    p->spc = spc;

    csound->AuxAlloc(csound, sizeof(float) * spc->ldim, &p->vec_scratch);
    csound->AuxAlloc(csound, sizeof(MYFLT) * p->ctx->ldim, &p->pmb);

    /* empty cache: forces the first add, and self-gates re-adds of unchanged text */
    csound->AuxAlloc(csound, 1, &p->last_text);
    ((char *) p->last_text.auxp)[0] = '\0';

    p->prev_trig = FL(0.0);

    return OK;
}

/* embed the sentence and append its vector(s) to the space. text longer than the model
   window is chunked (chunk_and_embed) and each chunk is added as a separate entry, like
   semspacebuild. a consecutive self-gate (last_text) skips re-embedding when the text is
   unchanged from the previous add (e.g. the same sentence on a repeated k-rate trigger).
   returns OK or NOTOK. shared by the i-rate and k-rate forms. */
static int sem_space_add_helper(CSOUND *csound, SEM_SPACE_ADD *p) {
    SEMSYS_SPACE *spc = p->spc;

    const char *text = (p->sentence != NULL && p->sentence->data != NULL) ? p->sentence->data : "";

    /* self-gate: skip when the text is unchanged from the previous add (no re-embed) */
    if (strcmp(text, (char *) p->last_text.auxp) == 0) {
        return OK;
    }

    MYFLT *pmb = (MYFLT *) p->pmb.auxp;
    float *pool = (float *) p->vec_scratch.auxp;   /* per-chunk scratch for chunk_and_embed */

    float *vecs = NULL;
    uint64_t count = 0;
    if (chunk_and_embed(text, NULL, &vecs, p->ctx, pmb, pool, &count) != OK) {
        free(vecs);
        return NOTOK;
    }

    for (uint64_t c = 0; c < count; c++) {
        if (spc->count == spc->capacity) {
            uint64_t new_cap = spc->capacity ? spc->capacity * 2 : INITIAL_VCAPACITY;
            float *tmp = (float *) csound->ReAlloc(csound, spc->vectors, sizeof(float) * new_cap * spc->ldim);
            if (tmp == NULL) {
                free(vecs);
                return NOTOK;
            }
            spc->vectors = tmp;
            spc->capacity = new_cap;
        }
        memcpy(spc->vectors + (spc->count * spc->ldim), vecs + c * spc->ldim, sizeof(float) * spc->ldim);
        spc->count++;
    }
    free(vecs);

    /* cache the text so an identical next add is gated out (grow buffer only on change) */
    size_t tlen = strlen(text) + 1;
    if (tlen > (size_t) p->last_text.size) {
        csound->AuxAlloc(csound, tlen, &p->last_text);
    }
    strcpy((char *) p->last_text.auxp, text);

    return OK;
}

/* k-rate form: add on the rising edge of ktrig */
int sem_space_add_perf(CSOUND *csound, SEM_SPACE_ADD *p) {
    MYFLT trig = *p->ktrig;
    if (trig > FL(0.0) && p->prev_trig <= FL(0.0)) {
        if (sem_space_add_helper(csound, p) == NOTOK) {
            p->prev_trig = trig;
            return csound->PerfError(csound, &(p->h), "[semspaceadd] add to space error");
        }
    }
    p->prev_trig = trig;
    return OK;
}

/* i-rate form: set up and add once, at init */
int sem_space_add(CSOUND *csound, SEM_SPACE_ADD *p) {
    int err = sem_space_add_init(csound, p);
    if (err != OK) {
        return err;
    }
    if (sem_space_add_helper(csound, p) == NOTOK) {
        return csound->InitError(csound, "[semspaceadd] add to space error");
    }
    return OK;
}

int sem_space_query_init(CSOUND *csound, SEM_SPACE_QUERY *p) {
    SEMSYS_SPACE *spc = FLT_TO_SPC(p);
    if (spc == NULL) {
        return csound->InitError(csound, "[semspacequery] Null space context");
    }

    *p->top_k = *p->top_k <= (MYFLT) spc->count ? *p->top_k : (MYFLT) spc->count;

    p->ctx = spc->ctx;
    p->spc = spc;
    p->top_k_neighs = (int) *p->top_k;
    tabinit_compat(csound, p->scores, p->top_k_neighs, &(p->h));
    tabinit2d(csound, p->neighs, p->top_k_neighs, spc->ldim, &(p->h));

    csound->AuxAlloc(csound, sizeof(float) * spc->ldim, &p->query_buf);

    csound->AuxAlloc(csound, sizeof(MYFLT) * p->ctx->ldim, &p->pmb);

    /* empty cache forces embedding on the first perf pass */
    csound->AuxAlloc(csound, 1, &p->last_text);
    ((char *) p->last_text.auxp)[0] = '\0';

    csound->AuxAlloc(csound, sizeof(Score) * p->top_k_neighs, &p->mheap);

    return OK;
}

static int sem_space_query_helper(SEM_SPACE_QUERY *p) {
    SEMSYS_SPACE *spc = p->spc;

    MYFLT *neighs = (MYFLT *) p->neighs->data;
    MYFLT *out_scores = (MYFLT *) p->scores->data;
    for (int i = 0; i < p->top_k_neighs; i++) {
        out_scores[i] = FL(0.0);
        for (uint32_t j = 0; j < spc->ldim; j++) {
            neighs[i * spc->ldim + j] = FL(0.0);
        }
    }

    MYFLT *pmb = (MYFLT *) p->pmb.auxp;
    float *query = (float *) p->query_buf.auxp;

    const char *raw_sentence = (p->query != NULL && p->query->data != NULL) ? p->query->data : "";
    const char *sentence = skip_leading_query_junk(raw_sentence);

    /* chunk the query and mean-pool the per-chunk embeddings into one centroid query
       vector, so a query longer than the model window is fully represented instead of
       truncated to its first ~maxlen tokens. query_buf doubles as chunk_and_embed's
       per-chunk scratch, then is overwritten with the mean below. */
    float *vecs = NULL;
    uint64_t count = 0;
    if (chunk_and_embed(sentence, NULL, &vecs, p->ctx, pmb, query, &count) != OK) {
        free(vecs);
        return NOTOK;
    }

    if (count == 0) {
        free(vecs);
        return OK;   /* empty query: neighbour/score outputs already zeroed above */
    }

    for (uint32_t j = 0; j < spc->ldim; j++) {
        double acc = 0.0;
        for (uint64_t c = 0; c < count; c++) {
            acc += (double) vecs[c * spc->ldim + j];
        }
        query[j] = (float) (acc / (double) count);
    }
    free(vecs);

    if (normalize(query, spc->ldim) != OK) {
        return OK;
    }

    if (spc->count == 0) { return OK; }

    Score *mheap = (Score *) p->mheap.auxp;
    int mheap_size = 0;
    for (uint64_t i = 0; i < spc->count; i++) {
        size_t offset = i * spc->ldim;
        float curr_score = dot(query, spc->vectors + offset, spc->ldim);
        if (!float_is_finite(curr_score)) {
            continue;
        }
        Score curr = { .ndx = i, .score = curr_score };
        if (mheap_size < p->top_k_neighs) {
            mheap[mheap_size] = curr;
            heapfy_up(mheap, mheap_size);
            mheap_size++;
        } else {
            if (curr.score > mheap[0].score) {
                mheap[0] = curr;
                heapfy_down(mheap, 0, mheap_size);
            }
        }
    }

    qsort(mheap, mheap_size, sizeof(Score), compare);

    for (int i = 0; i < p->top_k_neighs; i++) {
        if (i >= mheap_size) {
            continue;
        }
        Score neigh = mheap[i];
        out_scores[i] = (MYFLT) neigh.score;
        float *vec = spc->vectors + neigh.ndx * spc->ldim;
        for (uint32_t j = 0; j < spc->ldim; j++) {
            neighs[i * spc->ldim + j] = (MYFLT) vec[j];
        }
    }

    return OK;
}

int sem_space_query_perf(CSOUND *csound, SEM_SPACE_QUERY *p) {
    const char *sentence = (p->query != NULL && p->query->data != NULL) ? p->query->data : "";

    /* self-gate: skip the model when the query is unchanged */
    if (strcmp(sentence, (char *) p->last_text.auxp) == 0) {
        return OK;
    }

    size_t tlen = strlen(sentence) + 1;
    if (tlen > (size_t) p->last_text.size) {
        csound->AuxAlloc(csound, tlen, &p->last_text);
    }
    strcpy((char *) p->last_text.auxp, sentence);

    if (sem_space_query_helper(p) == NOTOK) {
        return csound->PerfError(csound, &(p->h), "[semspacequery] onnx model error");
    }

    return OK;
}

int sem_space_query_i(CSOUND *csound, SEM_SPACE_QUERY *p) {
    int err = sem_space_query_init(csound, p);
    if (err != OK) { return err; }

    if (sem_space_query_helper(p) == NOTOK) {
        return csound->InitError(csound, "[semspacequery] onnx model error");
    }

    return OK;
}

/* dump the whole space (header + all vectors) to a fresh file.
   returns OK, or NOTOK if the file cannot be created */
static int space_dump(SEMSYS_SPACE *spc, const char *path) {
    FILE *fptr = fopen(path, "wb");
    if (fptr == NULL) {
        return NOTOK;
    }

    CACHE_HEADER ch = {
        .count = spc->count,
        .ldim = spc->ldim,
        .head_tag = CACHE_HEADER_TAG,
        .data_tag = CACHE_DATA_TAG
    };

    fwrite(&ch, sizeof(CACHE_HEADER), 1, fptr);
    fwrite(spc->vectors, sizeof(float), spc->count * spc->ldim, fptr);
    fclose(fptr);

    return OK;
}

/* i-time form: semspacesave ispace, Sfile -> dump once at init */
int sem_space_save(CSOUND *csound, SEM_SPACE_SAVE *p) {
    SEMSYS_SPACE *spc = FLT_TO_SPC(p);
    if (spc == NULL) {
        return csound->InitError(csound, "[semspacesave] Null space context");
    }
    if (space_dump(spc, p->fname->data) != OK) {
        return csound->InitError(csound, "[semspacesave] Cannot create file");
    }
    return OK;
}

/* k-time form: semspacesave ispace, Sfile, ktrig -> dump on rising edge of ktrig */
int sem_space_save_kset(CSOUND *csound, SEM_SPACE_SAVE *p) {
    p->spc = FLT_TO_SPC(p);
    if (p->spc == NULL) {
        return csound->InitError(csound, "[semspacesave] Null space context");
    }
    p->prev_trig = FL(0.0);
    return OK;
}

int sem_space_save_kperf(CSOUND *csound, SEM_SPACE_SAVE *p) {
    MYFLT trig = *p->ktrig;
    if (trig > FL(0.0) && p->prev_trig <= FL(0.0)) {
        if (space_dump(p->spc, p->fname->data) != OK) {
            return csound->PerfError(csound, &(p->h), "[semspacesave] Cannot create file");
        }
    }
    p->prev_trig = trig;
    return OK;
}

/* fwd decl (defined below, used by sem_stt_init) */
static uintptr_t stt_worker(void *arg);

static void release_stt_ctx(CSOUND *csound, SEMSYS_STT *ctx) {
    if (ctx == NULL) {
        return;
    }
    if (STT_DEBUG_ENABLED && ctx->mutex != NULL) {
        csound->LockMutex(ctx->mutex);
        int pending_jobs = ctx->qcount;
        int pending_results = ctx->rcount;
        csound->UnlockMutex(ctx->mutex);
        if (pending_jobs > 0 || pending_results > 0) {
            csound->Message(csound, "[semstt] deinit drops pending jobs=%d results=%d\n", pending_jobs, pending_results);
        }
    }
    /* stop and join the worker BEFORE releasing the session it uses */
    if (ctx->thread != NULL) {
        if (ctx->mutex != NULL) {
            csound->LockMutex(ctx->mutex);
            ctx->stop = 1;
            csound->UnlockMutex(ctx->mutex);
        } else {
            ctx->stop = 1;
        }
        if (ctx->job_lock != NULL) {
            csound->NotifyThreadLock(ctx->job_lock);
        }
        csound->JoinThread(ctx->thread);
        ctx->thread = NULL;
        ctx->worker_done = 0;
    }
    if (ctx->job_lock != NULL) {
        csound->DestroyThreadLock(ctx->job_lock);
        ctx->job_lock = NULL;
    }
    if (ctx->mutex != NULL) {
        csound->DestroyMutex(ctx->mutex);
        ctx->mutex = NULL;
    }
    if (ctx->api != NULL) {
        if (ctx->mod_session != NULL) {
            ctx->api->ReleaseSession(ctx->mod_session);
            ctx->mod_session = NULL;
        }
        if (ctx->mod_session_options != NULL) {
            ctx->api->ReleaseSessionOptions(ctx->mod_session_options);
            ctx->mod_session_options = NULL;
        }
        if (ctx->env != NULL) {
            ctx->api->ReleaseEnv(ctx->env);
            ctx->env = NULL;
        }
    }
    /* drain the queues: job chunks and result texts are malloc'd (cross-thread) -> free() */
    if (ctx->jobs != NULL) {
        for (int i = 0; i < ctx->qcount; i++) {
            STT_JOB *j = &ctx->jobs[(ctx->qhead + i) % ctx->qcap];
            for (int c = 0; c < j->nchunks; c++) {
                free(j->chunks[c]);
            }
            free(j->chunks);
            free(j->sizes);
        }
        csound->Free(csound, ctx->jobs);
        ctx->jobs = NULL;
    }
    if (ctx->results != NULL) {
        for (int i = 0; i < ctx->rcount; i++) {
            free(ctx->results[(ctx->rhead + i) % ctx->qcap]);
        }
        csound->Free(csound, ctx->results);
        ctx->results = NULL;
    }
    if (ctx->result_ids != NULL) {
        csound->Free(csound, ctx->result_ids);
        ctx->result_ids = NULL;
    }
    csound->Free(csound, ctx);
}

static int32_t sem_stt_deinit(CSOUND *csound, void *vp) {
    SEM_STT_INIT *p = (SEM_STT_INIT *) vp;
    SEMSYS_STT *ctx = (SEMSYS_STT *) (uintptr_t) *p->handle;
    release_stt_ctx(csound, ctx);
    *p->handle = FL(0.0);
    return OK;
}

int sem_stt_init(CSOUND *csound, SEM_STT_INIT *p) {
    if (p->model_dir == NULL || p->model_dir->data == NULL) {
        return csound->InitError(csound, "[semsttload] Missing model dir");
    }
    int ret = OK;

    SEMSYS_STT *ctx = (SEMSYS_STT *) csound->Calloc(csound, sizeof(SEMSYS_STT));
    if (ctx == NULL) {
        return csound->InitError(csound, "[semsttload] Could not allocate context");
    }

    /* publish handle before anything can fail; cleanup runs via the API7 OENTRY deinit slot */
    *p->handle = STT_TO_FLT(ctx);

    const char *mdir = (const char *) p->model_dir->data;
    size_t mdlen = strlen(mdir);
    const char *sep = (mdlen > 0 && (mdir[mdlen - 1] == '/' || mdir[mdlen - 1] == '\\')) ? "" : "/";
    snprintf(ctx->model_path, sizeof(ctx->model_path), "%s%smodel.onnx", mdir, sep);

    // check if model files exists and are valid
    char model_data_path[1024];
    snprintf(model_data_path, sizeof(model_data_path), "%s%smodel.onnx.data", mdir, sep);
    FILE *fmcheck = fopen(ctx->model_path, "rb");
    FILE *fdcheck = fopen(model_data_path, "rb");
    if (fmcheck == NULL || fdcheck == NULL) {
        if (fmcheck != NULL) fclose(fmcheck);
        if (fdcheck != NULL) fclose(fdcheck);
        ret = csound->InitError(csound, "[semsttload] Missing model file: need model.onnx and model.onnx.data in %s", mdir);
        goto fail;
    }

    fclose(fmcheck);
    fclose(fdcheck);
    // ---

    ctx->api = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (ctx->api == NULL) {
        ret = csound->InitError(csound, "[semsttload] Could not get onnxruntime api");
        goto fail;
    }

    // maybe in check -> release stt
    ONNX_CHECK_STT(ctx->api, ctx->api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "SemSysSTT", &ctx->env));

    ONNX_CHECK_STT(ctx->api, ctx->api->CreateSessionOptions(&ctx->mod_session_options));
    /* the E2E whisper graph uses onnxruntime-extensions contrib ops
       (AudioDecoder, StftNorm, BpeDecoder) -> register the extensions library */
    char extbuf[1024];
    const char *extpath = resolve_extensions_path(mdir, extbuf, sizeof(extbuf));
    void *exthandle = NULL;
    ONNX_CHECK_STT(ctx->api, ctx->api->RegisterCustomOpsLibrary(ctx->mod_session_options, extpath, &exthandle));
    ONNX_CHECK_STT(ctx->api, create_session_utf8(ctx->api, ctx->env, ctx->model_path, ctx->mod_session_options, &ctx->mod_session));

    ctx->max_length = (int32_t) *p->max_length;

    /* fixed FIFO queues (input jobs + output results). the user value is the
       capacity; default is intentionally large enough for live chunk backlogs. */
    int depth = (int) *p->queue_depth;
    if (depth <= 0) {
        depth = STT_DEFAULT_QUEUE_CAP;
    }
    if (depth > STT_MAX_QUEUE_CAP) {
        depth = STT_MAX_QUEUE_CAP;
    }
    ctx->qcap = depth;
    ctx->jobs = (STT_JOB *) csound->Calloc(csound, sizeof(STT_JOB) * depth);
    ctx->results = (char **) csound->Calloc(csound, sizeof(char *) * depth);
    ctx->result_ids = (uint64_t *) csound->Calloc(csound, sizeof(uint64_t) * depth);
    ctx->next_job_id = 1;
    if (ctx->jobs == NULL || ctx->results == NULL || ctx->result_ids == NULL) {
        ret = csound->InitError(csound, "[semsttload] Could not allocate queues");
        goto fail;
    }

    ctx->csound = csound;
    ctx->stop = 0;
    ctx->mutex = csound->Create_Mutex(0);
    ctx->job_lock = csound->CreateThreadLock();
    ctx->thread = NULL;
    ctx->worker_done = 0;
    if (ctx->mutex == NULL || ctx->job_lock == NULL) {
        ret = csound->InitError(csound, "[semsttload] Could not start worker thread");
        goto fail;
    }
    return OK;

fail:
    sem_stt_deinit(csound, p);
    return ret;
}

/* run the E2E model on `audio` (audio_size bytes). on success *out_text is a malloc'd
   string. pure ORT + libc -> safe to call from the worker thread (no Csound calls,
   no InitError). returns OK / NOTOK (errbuf gets the message). does NOT free `audio`. */
static int stt_run(SEMSYS_STT *ctx, const uint8_t *audio, size_t audio_size, char **out_text, char *errbuf, size_t errcap) {
    const OrtApi *api = ctx->api;
    int ret = OK;
    OrtMemoryInfo *meminfo = NULL;
    OrtValue *audio_tensor = NULL;
    OrtValue *max_length_tensor = NULL;
    OrtValue *min_length_tensor = NULL;
    OrtValue *num_beams_tensor = NULL;
    OrtValue *num_return_sequences_tensor = NULL;
    OrtValue *length_penalty_tensor = NULL;
    OrtValue *repetition_penalty_tensor = NULL;
    OrtValue *outputs[1] = { NULL };
    char *sbuf = NULL;

    ORT_CK(api->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &meminfo));

    /* audio_stream tensor: uint8 [1, N]  (verify shape against the exported model).
       CreateTensorWithDataAsOrtValue does NOT copy -> `audio` must outlive the tensor. */
    int64_t audio_shape[2] = { 1, (int64_t) audio_size };
    ORT_CK(api->CreateTensorWithDataAsOrtValue(meminfo, (void *) audio, audio_size, audio_shape, 2, ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8, &audio_tensor));

    int32_t max_length = ctx->max_length;
    int32_t min_length = 0;
    int32_t num_beams = 1;
    int32_t num_return_sequences = 1;
    float length_penalty = 1.0f;
    float repetition_penalty = 1.0f;
    int64_t scalar_shape[1] = { 1 };

    ORT_CK(api->CreateTensorWithDataAsOrtValue(meminfo, &max_length, sizeof(int32_t), scalar_shape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32, &max_length_tensor));
    ORT_CK(api->CreateTensorWithDataAsOrtValue(meminfo, &min_length, sizeof(int32_t), scalar_shape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32, &min_length_tensor));
    ORT_CK(api->CreateTensorWithDataAsOrtValue(meminfo, &num_beams, sizeof(int32_t), scalar_shape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32, &num_beams_tensor));
    ORT_CK(api->CreateTensorWithDataAsOrtValue(meminfo, &num_return_sequences, sizeof(int32_t), scalar_shape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32, &num_return_sequences_tensor));
    ORT_CK(api->CreateTensorWithDataAsOrtValue(meminfo, &length_penalty, sizeof(float), scalar_shape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &length_penalty_tensor));
    ORT_CK(api->CreateTensorWithDataAsOrtValue(meminfo, &repetition_penalty, sizeof(float), scalar_shape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &repetition_penalty_tensor));

    const char *input_names[] = {
        "audio_stream",
        "max_length",
        "min_length",
        "num_beams",
        "num_return_sequences",
        "length_penalty",
        "repetition_penalty"
    };

    const OrtValue *inputs[] = {
        audio_tensor,
        max_length_tensor,
        min_length_tensor,
        num_beams_tensor,
        num_return_sequences_tensor,
        length_penalty_tensor,
        repetition_penalty_tensor
    };

    const char *output_names[] = { "str" };

    ORT_CK(api->Run(ctx->mod_session, NULL, input_names, inputs, 7, output_names, 1, outputs));

    /* the output string tensor may hold more than one element
       ([num_return_sequences, ...]); read ALL of them (GetStringTensorContent copies
       the elements back-to-back) -> need the true element count for offsets_count, or
       we'd only get the first element (a truncated transcription) */
    OrtTensorTypeAndShapeInfo *oinfo = NULL;
    ORT_CK(api->GetTensorTypeAndShape(outputs[0], &oinfo));
    size_t nstr = 0;
    {
        OrtStatus *st = api->GetTensorShapeElementCount(oinfo, &nstr);
        api->ReleaseTensorTypeAndShapeInfo(oinfo);
        if (st != NULL) {
            snprintf(errbuf, errcap, "onnxruntime: %s", api->GetErrorMessage(st));
            api->ReleaseStatus(st);
            ret = NOTOK;
            goto fail;
        }
    }

    if (nstr < 1) nstr = 1;

    size_t total = 0;
    ORT_CK(api->GetStringTensorDataLength(outputs[0], &total));
    sbuf = (char *) malloc(total + 1);
    if (sbuf == NULL) {
        snprintf(errbuf, errcap, "out of memory reading result");
        ret = NOTOK;
        goto fail;
    }

    size_t *offsets = (size_t *) malloc(sizeof(size_t) * nstr);
    if (offsets == NULL) {
        snprintf(errbuf, errcap, "out of memory reading result");
        ret = NOTOK;
        goto fail;
    }

    {
        OrtStatus *st = api->GetStringTensorContent(outputs[0], sbuf, total, offsets, nstr);
        free(offsets);
        if (st != NULL) {
            snprintf(errbuf, errcap, "onnxruntime: %s", api->GetErrorMessage(st));
            api->ReleaseStatus(st);
            ret = NOTOK;
            goto fail;
        }
    }

    sbuf[total] = '\0';
    *out_text = sbuf;
    sbuf = NULL;

fail:
    if (sbuf)                        free(sbuf);
    if (audio_tensor)                api->ReleaseValue(audio_tensor);
    if (max_length_tensor)           api->ReleaseValue(max_length_tensor);
    if (min_length_tensor)           api->ReleaseValue(min_length_tensor);
    if (num_beams_tensor)            api->ReleaseValue(num_beams_tensor);
    if (num_return_sequences_tensor) api->ReleaseValue(num_return_sequences_tensor);
    if (length_penalty_tensor)       api->ReleaseValue(length_penalty_tensor);
    if (repetition_penalty_tensor)   api->ReleaseValue(repetition_penalty_tensor);
    if (outputs[0])                  api->ReleaseValue(outputs[0]);
    if (meminfo)                     api->ReleaseMemoryInfo(meminfo);
    return ret;
}

/* push a job at the tail. returns 0 if the fixed queue is full. takes ownership of
   chunks/sizes (freed by the worker after transcription, or on deinit). */
static int job_push(SEMSYS_STT *ctx, uint8_t **chunks, size_t *sizes, int nchunks) {
    if (ctx->qcount >= ctx->qcap) {
        return 0;
    }
    int idx = (ctx->qhead + ctx->qcount) % ctx->qcap;
    ctx->jobs[idx].chunks = chunks;
    ctx->jobs[idx].sizes = sizes;
    ctx->jobs[idx].nchunks = nchunks;
    ctx->jobs[idx].id = ctx->next_job_id++;
    ctx->qcount++;
    if (STT_DEBUG_ENABLED && ctx->csound != NULL) {
        ctx->csound->Message(
            ctx->csound,
            "[semstt] enqueue job#%" PRIu64 " chunks=%d qcount=%d\n",
            ctx->jobs[idx].id,
            nchunks,
            ctx->qcount
        );
    }
    return 1;
}

/* pop the oldest job into *out. returns 0 if empty. */
static int job_pop(SEMSYS_STT *ctx, STT_JOB *out) {
    if (ctx->qcount == 0) {
        return 0;
    }
    *out = ctx->jobs[ctx->qhead];
    ctx->qhead = (ctx->qhead + 1) % ctx->qcap;
    ctx->qcount--;
    if (STT_DEBUG_ENABLED && ctx->csound != NULL) {
        ctx->csound->Message(ctx->csound, "[semstt] dequeue job#%" PRIu64 " chunks=%d qcount=%d\n", out->id, out->nchunks, ctx->qcount);
    }
    return 1;
}

/* push a result at the tail. returns 0 if the fixed queue is full. */
static int result_push(SEMSYS_STT *ctx, char *text, uint64_t id) {
    if (ctx->rcount >= ctx->qcap) {
        return 0;
    }
    int idx = (ctx->rhead + ctx->rcount) % ctx->qcap;
    ctx->results[idx] = text;
    ctx->result_ids[idx] = id;
    ctx->rcount++;
    if (STT_DEBUG_ENABLED && ctx->csound != NULL) {
        ctx->csound->Message(ctx->csound, "[semstt] result job#%" PRIu64 " len=%zu rcount=%d\n", id, text != NULL ? strlen(text) : 0, ctx->rcount);
    }
    return 1;
}

/* pop the oldest result (ownership to caller). returns NULL if empty. */
static char *result_pop(SEMSYS_STT *ctx) {
    if (ctx->rcount == 0) {
        return NULL;
    }
    char *t = ctx->results[ctx->rhead];
    uint64_t id = ctx->result_ids[ctx->rhead];
    ctx->rhead = (ctx->rhead + 1) % ctx->qcap;
    ctx->rcount--;
    if (STT_DEBUG_ENABLED && ctx->csound != NULL) {
        ctx->csound->Message(ctx->csound, "[semstt] pop result job#%" PRIu64 " rcount=%d\n", id, ctx->rcount);
    }
    return t;
}

/* append `add` to the malloc'd string *dst (grown with realloc), inserting a single
   space separator when *dst is non-empty. leading whitespace of `add` is skipped so
   Whisper's per-chunk leading space does not pile up at the joins. returns NOTOK on
   OOM (leaving *dst intact). */
static int stt_join_text(char **dst, size_t *len, const char *add) {
    if (add == NULL) {
        return OK;
    }
    while (*add == ' ' || *add == '\t' || *add == '\n' || *add == '\r') {
        add++;
    }
    size_t al = strlen(add);
    if (al == 0) {
        return OK;
    }
    size_t sep = (*len > 0) ? 1 : 0;
    char *n = (char *) realloc(*dst, *len + sep + al + 1);
    if (n == NULL) {
        return NOTOK;
    }
    if (sep) {
        n[*len] = ' ';
    }
    memcpy(n + *len + sep, add, al);
    *len += sep + al;
    n[*len] = '\0';
    *dst = n;
    return OK;
}

static char *stt_error_text(const char *err) {
    const char *prefix = "[semstt error] ";
    const char *msg = (err != NULL && err[0] != '\0') ? err : "unknown worker error";
    size_t n = strlen(prefix) + strlen(msg) + 1;
    char *out = (char *) malloc(n);
    if (out == NULL) {
        return NULL;
    }
    snprintf(out, n, "%s%s", prefix, msg);
    return out;
}

/* worker thread: drain the job queue, transcribe each (heavy, off the audio thread),
   push the texts to the result queue in order. one per handle; exits on ctx->stop. */
static uintptr_t stt_worker(void *arg) {
    SEMSYS_STT *ctx = (SEMSYS_STT *) arg;
    CSOUND *csound = ctx->csound;
    int idle_ticks = 0;

    for (;;) {
        /* wake on notify, or every 100 ms (timeout avoids a lost-notify deadlock) */
        csound->WaitThreadLock(ctx->job_lock, STT_WAIT_MS);

        STT_JOB job;
        csound->LockMutex(ctx->mutex);
        if (ctx->stop) {
            ctx->worker_done = 1;
            csound->UnlockMutex(ctx->mutex);
            return 0;
        }
        int have = job_pop(ctx, &job);
        if (!have) {
            idle_ticks++;
            if (idle_ticks >= STT_IDLE_TICKS) {
                ctx->worker_done = 1;
                csound->UnlockMutex(ctx->mutex);
                return 0;
            }
            csound->UnlockMutex(ctx->mutex);
            continue;
        }
        idle_ticks = 0;
        csound->UnlockMutex(ctx->mutex);

        /* transcribe each chunk in order and join into one result. a single-chunk job
           (short audio, or an encoded file blob) behaves as before; a segmented job
           concatenates the per-chunk texts. per-chunk errors are non-fatal as long as
           at least one chunk succeeds. */
        char *combined = NULL;
        size_t clen = 0;
        int any_ok = 0;
        char lasterr[256] = { 0 };

        for (int c = 0; c < job.nchunks; c++) {
            char *text = NULL;
            char err[256] = { 0 };
            int ok = stt_run(ctx, job.chunks[c], job.sizes[c], &text, err, sizeof(err));
            free(job.chunks[c]);
            if (ok == OK) {
                if (stt_join_text(&combined, &clen, text) == OK) {
                    any_ok = 1;
                }
                free(text);
            } else {
                if (text) free(text);
                snprintf(lasterr, sizeof(lasterr), "%s", err);
                if (STT_DEBUG_ENABLED) {
                    csound->Message(csound, "[semstt] worker error job#%" PRIu64 " chunk %d/%d: %s\n", job.id, c + 1, job.nchunks, err);
                }
            }
        }
        free(job.chunks);
        free(job.sizes);

        csound->LockMutex(ctx->mutex);
        if (any_ok) {
            if (!result_push(ctx, combined, job.id)) {
                free(combined);
                csound->Message(csound, "[semstt warning] result queue full: dropping result job#%" PRIu64 "\n", job.id);
            }
        } else {
            if (combined) free(combined);
            snprintf(ctx->err, sizeof(ctx->err), "%s", lasterr);
            char *emsg = stt_error_text(lasterr);
            if (emsg != NULL) {
                if (!result_push(ctx, emsg, job.id)) {
                    free(emsg);
                    csound->Message(csound, "[semstt warning] result queue full: dropping error result job#%" PRIu64 "\n", job.id);
                }
            }
        }
        csound->UnlockMutex(ctx->mutex);
    }
}

static int stt_ensure_worker(CSOUND *csound, SEMSYS_STT *ctx) {
    if (ctx->mutex == NULL || ctx->job_lock == NULL) {
        return NOTOK;
    }

    void *thread = NULL;
    csound->LockMutex(ctx->mutex);
    if (ctx->thread != NULL && ctx->worker_done) {
        thread = ctx->thread;
        ctx->thread = NULL;
        ctx->worker_done = 0;
    }

    csound->UnlockMutex(ctx->mutex);
    if (thread != NULL) {
        csound->JoinThread(thread);
    }

    csound->LockMutex(ctx->mutex);
    if (ctx->thread == NULL) {
        ctx->stop = 0;
        ctx->worker_done = 0;
        ctx->thread = csound->CreateThread(stt_worker, ctx);
        if (ctx->thread == NULL) {
            csound->UnlockMutex(ctx->mutex);
            return NOTOK;
        }
    }
    csound->UnlockMutex(ctx->mutex);
    return OK;
}

static void stt_free_chunks(uint8_t **chunks, size_t *sizes, int nchunks) {
    if (chunks != NULL) {
        for (int i = 0; i < nchunks; i++) {
            free(chunks[i]);
        }
        free(chunks);
    }
    free(sizes);
}

/* hand malloc'd chunk buffers to the worker queue. returns immediately. a full queue
   drops the new submit with a warning and still returns OK; NOTOK means the worker
   could not be started. takes ownership of chunks/sizes in all cases. */
static int stt_enqueue_chunks(CSOUND *csound, SEMSYS_STT *ctx, uint8_t **chunks, size_t *sizes, int nchunks) {
    if (stt_ensure_worker(csound, ctx) != OK) {
        stt_free_chunks(chunks, sizes, nchunks);
        return NOTOK;
    }

    csound->LockMutex(ctx->mutex);
    int ok = job_push(ctx, chunks, sizes, nchunks);
    csound->UnlockMutex(ctx->mutex);
    if (!ok) {
        csound->Message(csound, "[semstt warning] queue full: dropping newest submit chunks=%d\n", nchunks);
        stt_free_chunks(chunks, sizes, nchunks);
        return OK;
    }

    csound->NotifyThreadLock(ctx->job_lock);   /* wake the worker */
    return OK;
}

/* single-buffer convenience: wrap one malloc'd blob as a 1-chunk job (used by the
   file path, which passes the encoded file as-is, and the live path, whose windows
   are already <=30s). takes ownership of `audio`. */
static int stt_enqueue(CSOUND *csound, SEMSYS_STT *ctx, uint8_t *audio, size_t n) {
    uint8_t **chunks = (uint8_t **) malloc(sizeof(uint8_t *));
    size_t *sizes = (size_t *) malloc(sizeof(size_t));
    if (chunks == NULL || sizes == NULL) {
        free(chunks);
        free(sizes);
        free(audio);
        return NOTOK;
    }
    chunks[0] = audio;
    sizes[0] = n;
    return stt_enqueue_chunks(csound, ctx, chunks, sizes, 1);
}

/* defined below (after pcm_to_wav); used by the file path to segment decoded WAV */
static int stt_segment_pcm(const MYFLT *s, size_t n, uint32_t sr,
                           uint8_t ***out_chunks, size_t **out_sizes, int *out_n);

/* little-endian readers (host is LE; pcm_to_wav writes LE too) */
static uint16_t rd_u16(const uint8_t *p) { uint16_t v; memcpy(&v, p, 2); return v; }
static uint32_t rd_u32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static int16_t  rd_i16(const uint8_t *p) { int16_t  v; memcpy(&v, p, 2); return v; }

/* parse a PCM16 RIFF/WAVE blob into mono MYFLT samples at the file's sample rate.
   walks the chunk list (fmt/data need not be adjacent). returns OK and fills
   out/out_n/out_sr (caller frees *out), or NOTOK if it is not a PCM16 WAV we can
   slice -> the caller then falls back to the raw-bytes passthrough. */
static int wav_to_pcm_mono(const uint8_t *b, size_t n, MYFLT **out, size_t *out_n, uint32_t *out_sr) {
    if (n < 44 || memcmp(b, "RIFF", 4) != 0 || memcmp(b + 8, "WAVE", 4) != 0) {
        return NOTOK;
    }

    uint16_t fmt = 0, ch = 0, bits = 0;
    uint32_t sr = 0;
    const uint8_t *data = NULL;
    size_t data_len = 0;
    int have_fmt = 0;

    size_t pos = 12;
    while (pos + 8 <= n) {
        uint32_t csz = rd_u32(b + pos + 4);
        const uint8_t *body = b + pos + 8;
        if (csz > n - pos - 8) csz = (uint32_t) (n - pos - 8);   /* clamp truncated */
        if (memcmp(b + pos, "fmt ", 4) == 0 && csz >= 16) {
            fmt  = rd_u16(body + 0);
            ch   = rd_u16(body + 2);
            sr   = rd_u32(body + 4);
            bits = rd_u16(body + 14);
            have_fmt = 1;
        } else if (memcmp(b + pos, "data", 4) == 0) {
            data = body;
            data_len = csz;
        }
        pos += 8 + csz + (csz & 1);   /* chunks are word-aligned */
    }

    if (!have_fmt || data == NULL || fmt != 1 || bits != 16 || ch < 1 || sr == 0) {
        return NOTOK;   /* non-PCM16 / extensible / compressed -> let the graph decode */
    }

    size_t nframes = data_len / (size_t) (2 * ch);
    if (nframes == 0) {
        return NOTOK;
    }

    MYFLT *pcm = (MYFLT *) malloc(sizeof(MYFLT) * nframes);
    if (pcm == NULL) {
        return NOTOK;
    }
    for (size_t i = 0; i < nframes; i++) {
        double acc = 0.0;
        for (uint16_t c = 0; c < ch; c++) {
            acc += (double) rd_i16(data + (i * ch + c) * 2);   /* downmix channels */
        }
        pcm[i] = (MYFLT) (acc / ((double) ch * 32768.0));
    }

    *out = pcm;
    *out_n = nframes;
    *out_sr = sr;
    return OK;
}

int sem_stt_submit_audio_file(CSOUND *csound, SEM_STT_SUBMIT_AUDIO_FILE *p) {
    SEMSYS_STT *ctx = FLT_TO_STT(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semsttsubmit] Null STT context");
    }
    if (p->audio_speech_fpath == NULL || p->audio_speech_fpath->data == NULL) {
        return csound->InitError(csound, "[semsttsubmitfile] Missing audio file path");
    }

    /* read the whole file as RAW bytes. */
    FILE *f = fopen(p->audio_speech_fpath->data, "rb");
    if (f == NULL) {
        return csound->InitError(csound, "[semsttsubmit] Cannot open audio file: %s", p->audio_speech_fpath->data);
    }
    fseek(f, 0, SEEK_END);
    long fsz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsz <= 0) {
        fclose(f);
        return csound->InitError(csound, "[semsttsubmit] Empty audio file");
    }
    uint8_t *audio = (uint8_t *) malloc((size_t) fsz);   /* worker frees -> plain malloc */
    if (audio == NULL) {
        fclose(f);
        return csound->InitError(csound, "[semsttsubmit] Out of memory reading audio");
    }
    size_t got = fread(audio, 1, (size_t) fsz, f);
    fclose(f);
    if (got != (size_t) fsz) {
        free(audio);
        return csound->InitError(csound, "[semsttsubmit] Short read on audio file");
    }

    /* only PCM16 WAV is accepted: decode + segment into <=30s chunks so long files
       transcribe fully. for other formats (mp3/flac/ogg) convert to WAV first, or load
       into an ftable and use semsttsubmitft. */
    MYFLT *pcm = NULL;
    size_t nframes = 0;
    uint32_t wsr = 0;
    int parsed = wav_to_pcm_mono(audio, (size_t) fsz, &pcm, &nframes, &wsr);
    free(audio);
    if (parsed != OK) {
        return csound->InitError(csound, "[semsttsubmitfile] Only PCM16 WAV is supported: %s", p->audio_speech_fpath->data);
    }

    uint8_t **chunks = NULL;
    size_t *sizes = NULL;
    int nchunks = 0;
    int seg = stt_segment_pcm(pcm, nframes, wsr, &chunks, &sizes, &nchunks);
    free(pcm);
    if (seg != OK) {
        return csound->InitError(csound, "[semsttsubmit] Out of memory building WAV");
    }
    if (stt_enqueue_chunks(csound, ctx, chunks, sizes, nchunks) != OK) {
        return csound->InitError(csound, "[semsttsubmit] Could not start transcription worker");
    }
    return OK;
}

/* wrap raw PCM samples (engine sr, mono) into an in-RAM 16-bit WAV.
   malloc'd so the worker can free it; caller passes it to stt_enqueue. */
static uint8_t *pcm_to_wav(const MYFLT *s, int n, uint32_t sr, size_t *out_size) {
    uint32_t data_bytes = (uint32_t) n * 2;
    size_t total_size = 44 + data_bytes;
    uint8_t *w = (uint8_t *) malloc(total_size);

    if (w == NULL) return NULL;

    uint32_t byte_rate = sr * 2;
    uint32_t chunk = 36 + data_bytes;
    uint32_t s16 = 16;
    uint16_t pcm = 1;
    uint16_t ch = 1;
    uint16_t ba = 2;
    uint16_t bps = 16;

    memcpy(w, "RIFF", 4);
    memcpy(w + 4, &chunk, 4);
    memcpy(w + 8, "WAVE", 4);
    memcpy(w + 12, "fmt ", 4);
    memcpy(w + 16, &s16, 4);
    memcpy(w + 20, &pcm, 2);
    memcpy(w + 22, &ch, 2);
    memcpy(w + 24, &sr, 4);
    memcpy(w + 28, &byte_rate, 4);
    memcpy(w + 32, &ba, 2);
    memcpy(w + 34, &bps, 2);
    memcpy(w + 36, "data", 4);
    memcpy(w + 40, &data_bytes, 4);

    int16_t *pcm16 = (int16_t *) (w + 44);
    for (int i = 0; i < n; i++) {
        MYFLT v = s[i];
        if (v >  FL(1.0)) v =  FL(1.0);
        if (v < -FL(1.0)) v = -FL(1.0);
        pcm16[i] = (int16_t) lrint(v * 32767.0);
    }

    *out_size = total_size;
    return w;
}

/* pick a cut point for a long PCM buffer: the quietest 20ms frame in the search
   window ending at target_end (so a boundary lands in a pause, not mid-word). the
   window never reaches past target_end, so the returned index stays <= target_end
   and the resulting chunk stays under the hard cap. returns a sample index in
   (pos, target_end]. */
static size_t stt_find_split(const MYFLT *s, size_t pos, size_t target_end, size_t search, uint32_t sr) {
    size_t frame = (size_t) ((double) sr * STT_LIVE_VAD_FRAME_SEC);
    if (frame < 1) frame = 1;
    size_t lo = (target_end > pos + search) ? (target_end - search) : (pos + 1);
    double best_rms = -1.0;
    size_t best = target_end;
    for (size_t f = lo; f + frame <= target_end; f += frame) {
        double sumsq = 0.0;
        for (size_t i = f; i < f + frame; i++) {
            double v = (double) s[i];
            sumsq += v * v;
        }
        double rms = sqrt(sumsq / (double) frame);
        if (best_rms < 0.0 || rms < best_rms) {
            best_rms = rms;
            best = f + frame / 2;   /* cut in the middle of the quietest frame */
        }
    }
    if (best <= pos || best > target_end) {
        best = target_end;
    }
    return best;
}

/* split raw PCM (engine sr, mono) into <=30s segments, each wrapped as an in-RAM WAV.
   Whisper only sees the first ~30s of any single input, so long audio must be cut and
   transcribed piecewise. boundaries snap to silence (see stt_find_split). on success
   fills out_chunks and out_sizes (malloc'd arrays the caller owns) and out_n >= 1.
   returns NOTOK on OOM (nothing allocated). */
static int stt_segment_pcm(const MYFLT *s, size_t n, uint32_t sr,
                           uint8_t ***out_chunks, size_t **out_sizes, int *out_n) {
    size_t max_samples = (size_t) (STT_LIVE_HARD_MAX_SEC * (double) sr);
    size_t target = (size_t) (STT_FILE_CHUNK_TARGET_SEC * (double) sr);
    size_t search = (size_t) (STT_FILE_CHUNK_SEARCH_SEC * (double) sr);
    if (max_samples < 1) max_samples = 1;
    if (target < 1) target = 1;
    if (target > max_samples) target = max_samples;

    int cap = 8, cnt = 0;
    uint8_t **chunks = (uint8_t **) malloc(sizeof(uint8_t *) * cap);
    size_t *sizes = (size_t *) malloc(sizeof(size_t) * cap);
    if (chunks == NULL || sizes == NULL) {
        free(chunks);
        free(sizes);
        return NOTOK;
    }

    size_t pos = 0;
    while (pos < n) {
        size_t end;
        if (n - pos <= max_samples) {
            end = n;
        } else {
            end = stt_find_split(s, pos, pos + target, search, sr);
            if (end <= pos) end = pos + target;
            if (end - pos > max_samples) end = pos + max_samples;
            if (end > n) end = n;
        }

        if (cnt == cap) {
            int ncap = cap * 2;
            uint8_t **nc = (uint8_t **) realloc(chunks, sizeof(uint8_t *) * ncap);
            if (nc != NULL) chunks = nc;
            size_t *ns = (nc != NULL) ? (size_t *) realloc(sizes, sizeof(size_t) * ncap) : NULL;
            if (ns != NULL) sizes = ns;
            if (nc == NULL || ns == NULL) {
                for (int i = 0; i < cnt; i++) free(chunks[i]);
                free(chunks);
                free(sizes);
                return NOTOK;
            }
            cap = ncap;
        }

        size_t wsz = 0;
        uint8_t *w = pcm_to_wav(s + pos, (int) (end - pos), sr, &wsz);
        if (w == NULL) {
            for (int i = 0; i < cnt; i++) free(chunks[i]);
            free(chunks);
            free(sizes);
            return NOTOK;
        }
        chunks[cnt] = w;
        sizes[cnt] = wsz;
        cnt++;
        pos = end;
    }

    *out_chunks = chunks;
    *out_sizes = sizes;
    *out_n = cnt;
    return OK;
}

int sem_stt_submit_audio_arr(CSOUND *csound, SEM_STT_SUBMIT_AUDIO_ARRAY *p) {
    SEMSYS_STT *ctx = FLT_TO_STT(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semsttsubmit] Null STT context");
    }
    if (p->audio_speech_arr == NULL || p->audio_speech_arr->data == NULL) {
        return csound->InitError(csound, "[semsttsubmit] No audio data from array");
    }

    int n = p->audio_speech_arr->sizes[0];
    if (n <= 0) {
        return csound->InitError(csound, "[semsttsubmit] Empty audio array");
    }
    uint32_t sr = (uint32_t) csound->GetEngineSr(csound);   /* samples are at engine sr */

    uint8_t **chunks = NULL;
    size_t *sizes = NULL;
    int nchunks = 0;
    if (stt_segment_pcm(p->audio_speech_arr->data, (size_t) n, sr, &chunks, &sizes, &nchunks) != OK) {
        return csound->InitError(csound, "[semsttsubmit] Out of memory building WAV");
    }

    if (stt_enqueue_chunks(csound, ctx, chunks, sizes, nchunks) != OK) {
        return csound->InitError(csound, "[semsttsubmit] Could not start transcription worker");
    }
    return OK;
}

int sem_stt_submit_audio_func(CSOUND *csound, SEM_STT_SUBMIT_AUDIO_FUNC *p) {
    SEMSYS_STT *ctx = FLT_TO_STT(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semsttsubmit] Null STT context");
    }

    FUNC *ft = csound->FTFind(csound, p->ftable_num);
    if (ft == NULL || ft->ftable == NULL) {
        return csound->InitError(csound, "[semsttsubmit] No audio data from ftable");
    }

    int n = (int) ft->flen;
    if (n <= 0) {
        return csound->InitError(csound, "[semsttsubmit] Empty ftable");
    }
    uint32_t sr = (uint32_t) csound->GetEngineSr(csound);   /* samples are at engine sr */

    uint8_t **chunks = NULL;
    size_t *sizes = NULL;
    int nchunks = 0;
    if (stt_segment_pcm(ft->ftable, (size_t) n, sr, &chunks, &sizes, &nchunks) != OK) {
        return csound->InitError(csound, "[semsttsubmit] Out of memory building WAV");
    }

    if (stt_enqueue_chunks(csound, ctx, chunks, sizes, nchunks) != OK) {
        return csound->InitError(csound, "[semsttsubmit] Could not start transcription worker");
    }
    return OK;
}

static STT_LIVE_ANALYSIS stt_analyze_live_window(const MYFLT *s, size_t n, uint32_t sr) {
    STT_LIVE_ANALYSIS a;
    memset(&a, 0, sizeof(a));
    a.first_voice = n;

    double total_sumsq = 0.0;
    for (size_t i = 0; i < n; i++) {
        double v = (double) s[i];
        double av = fabs(v);
        if (av > a.peak) a.peak = av;
        total_sumsq += v * v;
    }

    a.rms = n > 0 ? sqrt(total_sumsq / (double) n) : 0.0;

    size_t frame = (size_t) ((double) sr * STT_LIVE_VAD_FRAME_SEC);
    if (frame < 1) frame = 1;
    for (size_t pos = 0; pos < n; pos += frame) {
        size_t end = pos + frame;
        if (end > n) {
            end = n;
        }
        double sumsq = 0.0;
        double peak = 0.0;
        for (size_t i = pos; i < end; i++) {
            double v = (double) s[i];
            double av = fabs(v);
            if (av > peak) {
                peak = av;
            }
            sumsq += v * v;
        }
        double rms = sqrt(sumsq / (double) (end - pos));
        if (rms >= STT_LIVE_VAD_RMS || peak >= STT_LIVE_VAD_PEAK) {
            if (!a.has_voice) a.first_voice = pos;
            a.has_voice = 1;
            a.last_voice_end = end;
            a.voiced_samples += (end - pos);
        }
    }
    return a;
}

static void stt_keep_live_tail(SEM_STT_SUBMIT_LIVE *p, size_t start) {
    if (start >= p->len) {
        p->len = 0;
        return;
    }
    MYFLT *buf = (MYFLT *) p->buf.auxp;
    size_t keep = p->len - start;
    memmove(buf, buf + start, sizeof(MYFLT) * keep);
    p->len = keep;
}

/* snapshot a functional live window into a WAV and enqueue it.
   mode 0: automatic, submit only after enough speech and trailing silence.
   mode 1: forced boundary (trigger or hard cap), submit if there is enough speech.
   mode 2: final flush at deinit, submit if there is enough speech, otherwise drop. */
static void stt_submit_window(CSOUND *csound, SEM_STT_SUBMIT_LIVE *p, int mode) {
    uint32_t sr = (uint32_t) csound->GetEngineSr(csound);
    const MYFLT *samples = (const MYFLT *) p->buf.auxp;

    /* ignore windows too short to transcribe (e.g. a trigger right after start, when only
       the first control block has accumulated): keep accumulating instead of submitting a
       near-empty buffer that produces garbage / can crash the model. still submit once the
       buffer is full, so a too-small imaxdur never stalls. */
    size_t min_len = (size_t) (FL(0.5) * (MYFLT) sr);
    if (p->len < min_len && p->len < p->cap) {
        return;
    }

    STT_LIVE_ANALYSIS a = stt_analyze_live_window(samples, p->len, sr);
    size_t min_speech = (size_t) (STT_LIVE_MIN_SPEECH_SEC * (double) sr);

    if (min_speech > p->target / 2 && p->target > 0) min_speech = p->target / 2;

    size_t trailing = (a.has_voice && p->len > a.last_voice_end) ? (p->len - a.last_voice_end) : 0;
    size_t need_silence = (size_t) (STT_LIVE_TRAILING_SILENCE_SEC * (double) sr);

    if (!a.has_voice || a.voiced_samples < min_speech) {
        if (mode == 2 || !a.has_voice) {
            if (STT_DEBUG_ENABLED) {
                csound->Message(
                    csound,
                    "[semstt] live window dropped: insufficient speech samples=%zu sec=%.3f rms=%.6f peak=%.6f voiced=%.3f mode=%d\n",
                    p->len,
                    (double) p->len / (double) sr,
                    a.rms,
                    a.peak,
                    (double) a.voiced_samples / (double) sr,
                    mode
                );
            }
            p->len = 0;
            return;
        }
        size_t pad = (size_t) (STT_LIVE_PAD_BEFORE_SEC * (double) sr);
        size_t keep_start = (a.first_voice > pad) ? (a.first_voice - pad) : 0;
        if (mode != 0 || p->len >= p->cap) {
            if (keep_start == 0 && p->len >= p->cap) {
                if (STT_DEBUG_ENABLED) {
                    csound->Message(csound, "[semstt] live window dropped: hard cap without enough speech samples=%zu sec=%.3f rms=%.6f peak=%.6f voiced=%.3f\n",
                                    p->len, (double) p->len / (double) sr, a.rms, a.peak,
                                    (double) a.voiced_samples / (double) sr);
                }
                p->len = 0;
            } else {
                stt_keep_live_tail(p, keep_start);
            }
        }
        return;
    }

    if (mode == 0 && trailing < need_silence) {
        return;
    }

    if (STT_DEBUG_ENABLED) {
        csound->Message(
            csound,
            "[semstt] live window submit samples=%zu sec=%.3f rms=%.6f peak=%.6f voiced=%.3f trailing=%.3f mode=%d\n",
            p->len, (double) p->len / (double) sr,
            a.rms,
            a.peak,
            (double) a.voiced_samples / (double) sr,
            (double) trailing / (double) sr,
            mode
        );
    }

    size_t pad_before = (size_t) (STT_LIVE_PAD_BEFORE_SEC * (double) sr);
    size_t pad_after = (size_t) (STT_LIVE_PAD_AFTER_SEC * (double) sr);
    size_t start = (a.first_voice > pad_before) ? (a.first_voice - pad_before) : 0;
    size_t end = a.last_voice_end + pad_after;
    if (end > p->len) {
        end = p->len;
    }
    if (end <= start) {
        p->len = 0;
        return;
    }

    size_t wav_size = 0;
    uint8_t *audio = pcm_to_wav(samples + start, (int) (end - start), sr, &wav_size);
    p->len = 0;
    if (audio == NULL) {
        return;
    }
    if (stt_enqueue(csound, p->ctx, audio, wav_size) != OK && STT_DEBUG_ENABLED) {
        csound->Message(csound, "[semstt] live window dropped: worker unavailable\n");
    }
}

/* live a-rate: allocate the accumulation buffer (imaxdur seconds) */
int sem_stt_submit_live_init(CSOUND *csound, SEM_STT_SUBMIT_LIVE *p) {
    SEMSYS_STT *ctx = FLT_TO_STT(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semsttsubmit] Null STT context");
    }
    p->ctx = ctx;
    MYFLT dur = *p->imaxdur;
    if (dur <= FL(0.0)) {
        dur = FL(30.0);   /* default 30 s window (Whisper chunk length) */
    }
    uint32_t sr = (uint32_t) csound->GetEngineSr(csound);
    p->target = (size_t) (dur * (MYFLT) sr);
    if (p->target < 1) {
        p->target = 1;
    }
    size_t hard_cap = (size_t) (STT_LIVE_HARD_MAX_SEC * (double) sr);
    p->cap = (p->target > hard_cap) ? p->target : hard_cap;
    if (p->cap < p->target) {
        p->cap = p->target;
    }
    csound->AuxAlloc(csound, sizeof(MYFLT) * p->cap, &p->buf);
    p->len = 0;
    p->next_check = p->target;
    p->prev_trig = FL(0.0);
    return OK;
}

int sem_stt_submit_live_deinit(CSOUND *csound, SEM_STT_SUBMIT_LIVE *p) {
    if (p->ctx != NULL && p->buf.auxp != NULL && p->len > 0) {
        stt_submit_window(csound, p, 2);
    }
    return OK;
}

/* live a-rate: append this block's samples; submit the window on a ktrig rising edge
   or when the buffer fills */
int sem_stt_submit_live_perf(CSOUND *csound, SEM_STT_SUBMIT_LIVE *p) {
    uint32_t nsmps = CS_KSMPS;
    const MYFLT *asig = p->asig;
    MYFLT *buf = (MYFLT *) p->buf.auxp;
    size_t vad_frame = (size_t) (STT_LIVE_VAD_FRAME_SEC * csound->GetEngineSr(csound));
    if (vad_frame < 1) {
        vad_frame = 1;
    }

    uint32_t i = 0;
    while (i < nsmps) {
        if (p->len >= p->cap) {
            stt_submit_window(csound, p, 1);
            p->next_check = p->target;
        }
        size_t room = p->cap - p->len;
        if (room == 0) {
            break;
        }
        size_t todo = (size_t) (nsmps - i);
        if (todo > room) {
            todo = room;
        }
        memcpy(buf + p->len, asig + i, sizeof(MYFLT) * todo);
        p->len += todo;
        i += (uint32_t) todo;
        if (p->len >= p->cap) {           /* hard cap -> force a window */
            stt_submit_window(csound, p, 1);
            p->next_check = p->target;
        } else if (p->len >= p->next_check) { /* preferred cap -> check usable boundary */
            stt_submit_window(csound, p, 0);
            p->next_check = (p->len == 0) ? p->target : p->len + vad_frame;
        }
    }

    MYFLT trig = (p->ktrig != NULL) ? *p->ktrig : FL(0.0);
    if (trig > FL(0.0) && p->prev_trig <= FL(0.0)) {
        stt_submit_window(csound, p, 1);
        p->next_check = (p->len == 0) ? p->target : p->len + vad_frame;
    }
    p->prev_trig = trig;
    return OK;
}

/* k-rate poll: 1 when a fresh transcription is ready, else 0 */
/* k-rate poll: 1 when at least one transcription is waiting to be read */
int sem_stt_ready(CSOUND *csound, SEM_STT_READY *p) {
    SEMSYS_STT *ctx = FLT_TO_STT(p);
    if (ctx == NULL || ctx->mutex == NULL) {
        *p->is_ready = FL(0.0);
        return OK;
    }
    csound->LockMutex(ctx->mutex);
    int n = ctx->rcount;
    csound->UnlockMutex(ctx->mutex);
    *p->is_ready = (n > 0) ? FL(1.0) : FL(0.0);
    return OK;
}

/* k-rate: pop the oldest transcription (FIFO) into the string output. empty -> "" */
int sem_stt_result(CSOUND *csound, SEM_STT_RESULT *p) {
    SEMSYS_STT *ctx = FLT_TO_STT(p);
    if (ctx == NULL || ctx->mutex == NULL) {
        return csound->InitError(csound, "[semsttresult] Null STT context");
    }
    csound->LockMutex(ctx->mutex);
    char *t = result_pop(ctx);   /* ownership to us */
    csound->UnlockMutex(ctx->mutex);

    const char *txt = (t != NULL) ? t : "";
    size_t need = strlen(txt) + 1;
    if (p->result->data == NULL || (size_t) p->result->size < need) {
        p->result->data = (char *) csound->ReAlloc(csound, p->result->data, need);
        p->result->size = (int) need;
    }

    strcpy(p->result->data, txt);
    *p->text_length = (MYFLT) (need - 1);   /* char count: 0 when there is no text */
    if (t != NULL) {
        free(t);   /* consumed */
    }
    return OK;
}

#define S(s) sizeof(s)

static OENTRY localops[] = {
    { "semload",           S(SEM_INIT),                   0, "i",         "iS",   (SUBR)sem_init,                   NULL,                           (SUBR)sem_deinit,                 NULL, 0 },
    { "semdim",            S(SEM_DIM),                    0, "i",         "i",    (SUBR)sem_dim,                    NULL,                           NULL,                             NULL, 0 },
    { "semembed",          S(SEM_EMBED_TEXT_I),           0, "i[][]",     "iS",   (SUBR)sem_embed_i,                NULL,                           NULL,                             NULL, 0 },
    { "semembedfile",      S(SEM_EMBED_FILE_I),           0, "i[][]",     "iS",   (SUBR)sem_embed_i_file,           NULL,                           NULL,                             NULL, 0 },
    { "semembed.k",        S(SEM_EMBED),                  0, "k[]k",      "iS",   (SUBR)sem_embed_init,             (SUBR)sem_embed_perf,           NULL,                             NULL, 0 },
    { "semspace",          S(SEM_SPACE_INIT),             0, "i",         "i",    (SUBR)sem_space_init,             NULL,                           (SUBR)sem_space_deinit,           NULL, 0 },
    { "semspace.f",        S(SEM_SPACE_INIT),             0, "i",         "iS",   (SUBR)sem_space_init,             NULL,                           (SUBR)sem_space_deinit,           NULL, 0 },
    { "semspace.vs",       S(SEM_SPACE_INIT_VS),          0, "i",         "iS[]", (SUBR)sem_space_init_vs,          NULL,                           (SUBR)sem_space_deinit,           NULL, 0 },
    { "semspacebuild",     S(SEM_SPACE_BUILD),            0, "",          "iSS",  (SUBR)sem_space_build,            NULL,                           NULL,                             NULL, 0 },
    { "semspaceadd",       S(SEM_SPACE_ADD),              0, "",          "iS",   (SUBR)sem_space_add,              NULL,                           NULL,                             NULL, 0 },
    { "semspaceadd.k",     S(SEM_SPACE_ADD),              0, "",          "iSk",  (SUBR)sem_space_add_init,         (SUBR)sem_space_add_perf,       NULL,                             NULL, 0 },
    { "semspacesave",      S(SEM_SPACE_SAVE),             0, "",          "iS",   (SUBR)sem_space_save,             NULL,                           NULL,                             NULL, 0 },
    { "semspacesave.k",    S(SEM_SPACE_SAVE),             0, "",          "iSk",  (SUBR)sem_space_save_kset,        (SUBR)sem_space_save_kperf,     NULL,                             NULL, 0 },
    { "semspacequery",     S(SEM_SPACE_QUERY),            0, "i[][]i[]",  "iSi",  (SUBR)sem_space_query_i,          NULL,                           NULL,                             NULL, 0 },
    { "semspacequery.k",   S(SEM_SPACE_QUERY),            0, "k[][]k[]",  "iSi",  (SUBR)sem_space_query_init,       (SUBR)sem_space_query_perf,     NULL,                             NULL, 0 },
    { "semsttload",        S(SEM_STT_INIT),               0, "i",         "Sio",  (SUBR)sem_stt_init,               NULL,                           (SUBR)sem_stt_deinit,             NULL, 0 },
    { "semsttsubmitfile",  S(SEM_STT_SUBMIT_AUDIO_FILE),  0, "",          "iS",   (SUBR)sem_stt_submit_audio_file,  NULL,                           NULL,                             NULL, 0 },
    { "semsttsubmitarray", S(SEM_STT_SUBMIT_AUDIO_ARRAY), 0, "",          "ii[]", (SUBR)sem_stt_submit_audio_arr,   NULL,                           NULL,                             NULL, 0 },
    { "semsttsubmitft",    S(SEM_STT_SUBMIT_AUDIO_FUNC),  0, "",          "ii",   (SUBR)sem_stt_submit_audio_func,  NULL,                           NULL,                             NULL, 0 },
    { "semsttsubmitlive",  S(SEM_STT_SUBMIT_LIVE),        0, "",          "iaiO", (SUBR)sem_stt_submit_live_init,   (SUBR)sem_stt_submit_live_perf, (SUBR)sem_stt_submit_live_deinit, NULL, 0 },
    { "semsttready",       S(SEM_STT_READY),              0, "k",         "i",    NULL,                             (SUBR)sem_stt_ready,            NULL,                             NULL, 0 },
    { "semsttresult",      S(SEM_STT_RESULT),             0, "Sk",        "i",    NULL,                             (SUBR)sem_stt_result,           NULL,                             NULL, 0 },
};

LINKAGE
