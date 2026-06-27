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
#endif
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "semsys.h"
#include "../../common/_common.h"

// from else.c
static int32_t arraymake2d(CSOUND *csound, ARRAYDAT *arr, int numcols) {
    if(arr->dimensions != 1) {
        printf("arraymake2d: array is not 1D\n");
        return NOTOK;
    }
    int flatsize = arr->sizes[0];
    if(flatsize % numcols != 0) {
        printf("arraymale2d: array size %d is not divisible by colsize %d\n", flatsize, numcols);
        return NOTOK;
    }
    arr->sizes = csound->ReAlloc(csound, arr->sizes, sizeof(int32_t) * 2);
    arr->dimensions = 2;
    arr->sizes[0] = flatsize / numcols;
    arr->sizes[1] = numcols;
    return OK;
}

static int32_t tabinit2d(CSOUND *csound, ARRAYDAT *arr, int numrows, int numcols, OPDS *ctx) {
    int numelements = numrows * numcols;
    tabinit_compat(csound, arr, numelements, ctx);
    int res = arraymake2d(csound, arr, numcols);
    return res;
}
// -----

static inline void normalize(float *vec, uint32_t dim) {
    double norm = 0;
    for (uint32_t i = 0; i < dim; i++) {
        norm += (double)(vec[i] * vec[i]);
    }
    norm = sqrt(norm);
    if (norm != 0.0) {
        for (uint32_t i = 0; i < dim; i++) {
            vec[i] = (float)(vec[i] / norm);
        }
    }
}

static inline float dot(float *vec_a, float *vec_b, uint32_t dim) {
    double d = 0.0;
    for (uint32_t i = 0; i < dim; i++) {
        d += (double)(vec_a[i] * vec_b[i]);
    }
    return (float) d;
}

static char *read_line(FILE *fp) {
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

static char *read_paragraph(FILE *fp) {
    char *acc = NULL;
    size_t acc_len = 0;
    while (1) {
        char *line = read_line(fp);
        if (line == NULL) return acc;
        size_t llen = strlen(line);
        if (llen == 0) {
            free(line);
            if (acc == NULL) continue;
            return acc;
        }
        char *tmp = realloc(acc, acc_len + llen + 2);
        acc = tmp;
        if (tmp == NULL) {
            free(line);
            free(acc);
            return NULL;
        }
        if (acc_len > 0) acc[acc_len++] = ' ';
        memcpy(acc + acc_len, line, llen);
        acc_len += llen;
        acc[acc_len] = '\0';
        free(line);
    }
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
        if (ctx->tok_session != NULL) {
            ctx->api->ReleaseSession(ctx->tok_session);
            ctx->tok_session = NULL;
        }
        if (ctx->emb_session_options != NULL) {
            ctx->api->ReleaseSessionOptions(ctx->emb_session_options);
            ctx->emb_session_options = NULL;
        }
        if (ctx->tok_session_options != NULL) {
            ctx->api->ReleaseSessionOptions(ctx->tok_session_options);
            ctx->tok_session_options = NULL;
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

int sem_init(CSOUND *csound, SEM_INIT *p) {
    if (p->model_path == NULL || p->model_path->data == NULL) {
        return csound->InitError(csound, "[semload] Missing model path");
    }

    SEMSYS *ctx = (SEMSYS *) csound->Calloc(csound, sizeof(SEMSYS));
    if (ctx == NULL) {
        return csound->InitError(csound, "[sem_init] Could not allocate context");
    }

    ctx->maxlen_seq = (int32_t) *p->maxlen_seq;

    /* publish handle before anything can fail; cleanup runs via the API7 OENTRY deinit slot */
    *p->handle = CTX_TO_FLT(ctx);

    strncpy(ctx->model_path, p->model_path->data, sizeof(ctx->model_path) - 1);
    ctx->model_path[sizeof(ctx->model_path) - 1] = '\0';

    if (p->tokenizer_path != NULL && p->tokenizer_path->data != NULL) {
        strncpy(ctx->tokenizer_path, p->tokenizer_path->data, sizeof(ctx->tokenizer_path) - 1);
        ctx->tokenizer_path[sizeof(ctx->tokenizer_path) - 1] = '\0';
    } else {
        ctx->tokenizer_path[0] = '\0';
    }

    ctx->api = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (ctx->api == NULL) {
        return csound->InitError(csound, "[semload] Could not get onnxruntime api");
    }

    ONNX_CHECK_INIT(ctx->api, ctx->api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "SemSys", &ctx->env));

    ONNX_CHECK_INIT(ctx->api, ctx->api->CreateSessionOptions(&ctx->emb_session_options));
    ONNX_CHECK_INIT(ctx->api, ctx->api->CreateSession(ctx->env, ctx->model_path, ctx->emb_session_options, &ctx->emb_session));

    if (ctx->tokenizer_path[0] != '\0') {
        ONNX_CHECK_INIT(ctx->api, ctx->api->CreateSessionOptions(&ctx->tok_session_options));
        ONNX_CHECK_INIT(ctx->api, ctx->api->CreateSession(ctx->env, ctx->tokenizer_path, ctx->tok_session_options, &ctx->tok_session));
    }

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
}

int sem_dim(CSOUND *csound, SEM_DIM *p) {
    SEMSYS *ctx = FLT_TO_CTX(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semdim] Not valid handle");
    }

    *p->out_dim = (MYFLT) ctx->ldim;
    return OK;
}

static int tokenize_helper(MYFLT *input_ids, MYFLT *attention_mask, SEMSYS *ctx, uint32_t maxlen_seq, const char *text) {
    int64_t input_shape[] = { 1 };
    OrtAllocator *allocator = NULL;
    OrtValue *input_tensor = NULL;
    OrtValue *outputs[2] = { NULL, NULL };
    OrtTensorTypeAndShapeInfo *shape = NULL;
    int ret = 1;

    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetAllocatorWithDefaultOptions(&allocator));
    ONNX_CHECK_GOTO(ctx->api, ctx->api->CreateTensorAsOrtValue(allocator, input_shape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING, &input_tensor));
    ONNX_CHECK_GOTO(ctx->api, ctx->api->FillStringTensor(input_tensor, &text, 1));

    const char *input_names[] = { "text" };
    const OrtValue *inputs[] = { input_tensor };
    const char *output_names[] = { "input_ids", "attention_mask" };

    /* run first - outputs are only valid after Run */
    ONNX_CHECK_GOTO(ctx->api, ctx->api->Run(
        ctx->tok_session,
        NULL,
        input_names,
        inputs,
        1,
        output_names,
        2,
        outputs
    ));

    /* read shape from the produced output tensor */
    size_t ndims = 0;
    int64_t dims[8] = { 0 };
    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetTensorTypeAndShape(outputs[0], &shape));
    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetDimensionsCount(shape, &ndims));
    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetDimensions(shape, dims, ndims < 8 ? ndims : 8));
    ctx->api->ReleaseTensorTypeAndShapeInfo(shape);
    shape = NULL;

    int64_t seq_len = (ndims >= 2) ? dims[ndims - 1] : dims[0];

    int64_t *src_ids = NULL;
    int64_t *src_mask = NULL;
    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetTensorMutableData(outputs[0], (void **)&src_ids));
    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetTensorMutableData(outputs[1], (void **)&src_mask));

    /* copy into fixed arrays, truncate to maxlen, pad the rest with 0 */
    int32_t n = (seq_len < maxlen_seq) ? (int32_t) seq_len : (int32_t) maxlen_seq;
    for (int32_t i = 0; i < n; i++) {
        input_ids[i] = (MYFLT) src_ids[i];
        attention_mask[i] = (MYFLT) src_mask[i];
    }

    for (uint32_t i = n; i < maxlen_seq; i++) {
        input_ids[i] = FL(0.0);
        attention_mask[i] = FL(0.0);
    }

    ret = 0;

fail:
    if (shape != NULL) ctx->api->ReleaseTensorTypeAndShapeInfo(shape);
    if (outputs[0] != NULL) ctx->api->ReleaseValue(outputs[0]);
    if (outputs[1] != NULL) ctx->api->ReleaseValue(outputs[1]);
    if (input_tensor != NULL) ctx->api->ReleaseValue(input_tensor);
    return ret;
}

static int embed_helper(MYFLT *pool_embed, MYFLT *token_embed, const MYFLT *input_ids, const MYFLT *attention_mask, SEMSYS *ctx, int64_t n, uint32_t maxlen_seq) {
    int64_t shape[2] = { 1, n };
    OrtAllocator *alloc = NULL;
    OrtValue *ids_t = NULL, *mask_t = NULL;
    OrtValue *outs[1] = { NULL };
    int ret = 1;

    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetAllocatorWithDefaultOptions(&alloc));

    /* input int64: csound MYFLT -> int64 */
    ONNX_CHECK_GOTO(ctx->api, ctx->api->CreateTensorAsOrtValue(alloc, shape, 2, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, &ids_t));
    ONNX_CHECK_GOTO(ctx->api, ctx->api->CreateTensorAsOrtValue(alloc, shape, 2, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, &mask_t));

    int64_t *ids_p = NULL, *mask_p = NULL;
    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetTensorMutableData(ids_t,  (void **)&ids_p));
    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetTensorMutableData(mask_t, (void **)&mask_p));
    for (int64_t i = 0; i < n; i++) {
        ids_p[i]  = (int64_t) input_ids[i];
        mask_p[i] = (int64_t) attention_mask[i];
    }

    const char *in_names[]  = { "input_ids", "attention_mask" };
    const OrtValue *ins[]   = { ids_t, mask_t };
    const char *out_names[] = { "last_hidden_state" };

    ONNX_CHECK_GOTO(ctx->api, ctx->api->Run(ctx->emb_session, NULL, in_names, ins, 2, out_names, 1, outs));

    /* token embeddings float32 [n, ldim], row-major */
    float *tok = NULL;
    ONNX_CHECK_GOTO(ctx->api, ctx->api->GetTensorMutableData(outs[0], (void **)&tok));

    /* matrice token_embed (maxlen x ldim): riempi n righe, zero il resto */
    int32_t ldim = ctx->ldim;
    for (int64_t i = 0; i < n; i++)
        for (int32_t d = 0; d < ldim; d++)
            token_embed[i * ldim + d] = (MYFLT) tok[i * ldim + d];

    for (int64_t i = n; i < maxlen_seq; i++)
        for (int32_t d = 0; d < ldim; d++)
            token_embed[i * ldim + d] = FL(0.0);

    /* mean pooling -> pool_embed (ldim) */
    for (int32_t d = 0; d < ldim; d++) pool_embed[d] = FL(0.0);
    int64_t valid = 0;
    for (int64_t i = 0; i < n; i++) {
        if (mask_p[i] == 0) continue;
        valid++;
        for (int32_t d = 0; d < ldim; d++)
            pool_embed[d] += (MYFLT) tok[i * ldim + d];
    }
    if (valid > 0)
        for (int32_t d = 0; d < ldim; d++) pool_embed[d] /= (MYFLT) valid;

    ret = 0;

fail:
    if (outs[0] != NULL) ctx->api->ReleaseValue(outs[0]);
    if (ids_t != NULL) ctx->api->ReleaseValue(ids_t);
    if (mask_t != NULL) ctx->api->ReleaseValue(mask_t);
    return ret;
}

static int embed_sentence(MYFLT *ids, MYFLT *att, MYFLT *pmb, MYFLT *tmb, SEMSYS *ctx, const char *sentence) {
    int err;
    err = tokenize_helper(ids, att, ctx, ctx->maxlen_seq, sentence);
    if (err) return NOTOK;

    int64_t n = 0;
    for (uint32_t i = 0; i < ctx->maxlen_seq; i++) {
        if (att[i] != FL(0.0)) n++;
    }

    if (n == 0) { return n; }

    err = embed_helper(pmb, tmb, ids, att, ctx, n, ctx->maxlen_seq);
    if (err) return NOTOK;

    return n;
}

int sem_embed_init(CSOUND *csound, SEM_EMBED *p) {
    SEMSYS *ctx = FLT_TO_CTX(p);
    if (ctx == NULL) {
        return csound->InitError(csound, "[semembed] Not valid handle");
    }

    if (ctx->tok_session == NULL) {
        return csound->InitError(csound, "[semembed] No tokenizer session (missing tokenizer path)");
    }

    p->ctx = ctx;

    tabinit_compat(csound, p->pool_embed, ctx->ldim, &(p->h));
    tabinit2d(csound, p->token_embed, p->ctx->maxlen_seq, ctx->ldim, &(p->h));

    csound->AuxAlloc(csound, sizeof(MYFLT) * p->ctx->maxlen_seq, &(p->input_ids));
    csound->AuxAlloc(csound, sizeof(MYFLT) * p->ctx->maxlen_seq, &(p->attention_mask));

    /* empty cache forces tokenization on the first perf pass */
    csound->AuxAlloc(csound, 1, &p->last_text);
    ((char *) p->last_text.auxp)[0] = '\0';

    return OK;
}

int sem_embed_perf(CSOUND *csound, SEM_EMBED *p) {
    SEMSYS *ctx = p->ctx;
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

    MYFLT *ids = (MYFLT *) p->input_ids.auxp;
    MYFLT *att = (MYFLT *) p->attention_mask.auxp;
    MYFLT *pool_embed = p->pool_embed->data;
    MYFLT *token_embed = p->token_embed->data;

    int err = embed_sentence(ids, att, pool_embed, token_embed, ctx, text);
    if (err == NOTOK) {
        return csound->PerfError(csound, &(p->h), "[semembed] Embedding process error");
    }

    *p->gate = FL(1.0);

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
    csound->Free(csound, p->ids);
    csound->Free(csound, p->att);
    csound->Free(csound, p->pmb);
    csound->Free(csound, p->tmb);
    free(p->pool);
}

static uint64_t sem_space_create_helper(FILE *source_file, FILE *dest_file, SEMSYS *ctx, MYFLT *ids, MYFLT *att, MYFLT *pmb, MYFLT *tmb, float *pool) {
    uint64_t count = 0;
    char *paragraph;
    while ((paragraph = read_paragraph(source_file)) != NULL) {
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
                        word_start = realloc(word_start, cap * sizeof(size_t));
                    }
                    word_start[n_words++] = i;
                }
                in_word = 1;
            } else {
                in_word = 0;
            }
        }

        char *window = malloc(plen + 1);
        for (size_t i = 0; i < n_words; i+=stride) {
            size_t k = (i + wsize) < n_words ? (i + wsize) : n_words;
            size_t start = word_start[i];
            size_t end = (k < n_words) ? word_start[k] : plen;
            size_t wlen = end - start;
            memcpy(window, paragraph + start, wlen);
            window[wlen] = '\0';
            int err = embed_sentence(ids, att, pmb, tmb, ctx, window);
            if (err == NOTOK) {
                free(window);
                free(word_start);
                free(paragraph);
                return NOTOK;
            }

            if (err > 0) {
                for (uint32_t i = 0; i < ctx->ldim; i++) {
                    pool[i] = (float) pmb[i];
                }

                normalize(pool, ctx->ldim);

                fwrite(pool, sizeof(float), ctx->ldim, dest_file);
                count++;

            }

            if (k == n_words) break;
        }

        free(word_start);
        free(window);
        free(paragraph);
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

    size_t got = fread(spc->vectors + spc->count * spc->ldim, sizeof(float) * spc->ldim, ch.count, f);
    fclose(f);
    if (got != ch.count) return NOTOK;

    spc->count = new_count;
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

    p->ids = csound->Calloc(csound, sizeof(MYFLT) * ctx->maxlen_seq);
    p->att = csound->Calloc(csound, sizeof(MYFLT) * ctx->maxlen_seq);
    p->pmb = csound->Calloc(csound, sizeof(MYFLT) * ctx->ldim);
    p->tmb = csound->Calloc(csound, sizeof(MYFLT) * (ctx->ldim * ctx->maxlen_seq));
    p->pool = (float *) calloc(ctx->ldim, sizeof(float));

    int failed = 0;
    for (size_t i = 0; i < nfiles; i++) {
        FILE *cf = fopen(list[i], "rb");
        if (cf == NULL) { failed = 1; break; }
        uint64_t _count = sem_space_create_helper(cf, fptr, ctx, p->ids, p->att, p->pmb, p->tmb, p->pool);
        fclose(cf);
        if ((int) _count == NOTOK) { failed = 1; break; }
        ch.count += _count;
    }

    free_str_list(files, nfiles);

    if (failed) {
        sem_space_build_deinit(csound, p, fptr, NULL);
        return csound->InitError(csound, "[semspacebuild] Error processing source files");
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

    csound->AuxAlloc(csound, sizeof(MYFLT) * spc->ldim, &p->last_added);
    csound->AuxAlloc(csound, sizeof(float) * spc->ldim, &p->vec_scratch);

    csound->AuxAlloc(csound, sizeof(MYFLT) * p->ctx->maxlen_seq, &p->ids);
    csound->AuxAlloc(csound, sizeof(MYFLT) * p->ctx->maxlen_seq, &p->att);
    csound->AuxAlloc(csound, sizeof(MYFLT) * p->ctx->ldim, &p->pmb);
    csound->AuxAlloc(csound, sizeof(MYFLT) * (p->ctx->ldim * p->ctx->maxlen_seq), &p->tmb);

    /* empty cache forces embedding on the first perf pass */
    csound->AuxAlloc(csound, 1, &p->last_text);
    ((char *) p->last_text.auxp)[0] = '\0';

    return OK;
}

int sem_space_add(CSOUND *csound, SEM_SPACE_ADD *p) {

    SEMSYS_SPACE *spc = p->spc;
    const char *sentence = p->sentence->data;

    /* self-gate: skip the model when the sentence is unchanged */
    if (strcmp(sentence, (char *) p->last_text.auxp) == 0) {
        return OK;
    }
    size_t tlen = strlen(sentence) + 1;
    if (tlen > (size_t) p->last_text.size) {
        csound->AuxAlloc(csound, tlen, &p->last_text);
    }
    strcpy((char *) p->last_text.auxp, sentence);

    MYFLT *ids = (MYFLT *) p->ids.auxp;
    MYFLT *att = (MYFLT *) p->att.auxp;
    MYFLT *in_vec = (MYFLT *) p->pmb.auxp;
    MYFLT *tmb = (MYFLT *) p->tmb.auxp;

    int err = embed_sentence(ids, att, in_vec, tmb, p->ctx, sentence);
    if (err == NOTOK) {
        return csound->PerfError(csound, &(p->h), "[semspaceadd] onnx model error");
    };

    MYFLT *last = (MYFLT *) p->last_added.auxp;

    /* dedup: skip when identical to the previously added vector */
    if (memcmp(in_vec, last, sizeof(MYFLT) * spc->ldim) == 0) {
        return OK;
    }

    float *vec = (float *) p->vec_scratch.auxp;
    for (uint32_t i = 0; i < spc->ldim; i++) {
        vec[i] = (float) in_vec[i];
    }

    normalize(vec, spc->ldim);

    if (spc->count == spc->capacity) {
        uint64_t new_cap = spc->capacity ? spc->capacity * 2 : INITIAL_VCAPACITY;
        float *tmp = (float *) csound->ReAlloc(csound, spc->vectors, sizeof(float) *  new_cap * spc->ldim);
        if (tmp == NULL) {
            return csound->PerfError(csound, &(p->h), "[semspaceadd] Out of memory in realloc vector space");
        }

        spc->vectors = tmp;
        spc->capacity = new_cap;
    }

    memcpy(spc->vectors + (spc->count * spc->ldim), vec, sizeof(float) * spc->ldim);
    spc->count++;

    memcpy(last, in_vec, sizeof(MYFLT) * spc->ldim);

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

    csound->AuxAlloc(csound, sizeof(float) * p->ctx->maxlen_seq, &p->ids);
    csound->AuxAlloc(csound, sizeof(float) * p->ctx->maxlen_seq, &p->att);
    csound->AuxAlloc(csound, sizeof(float) * p->ctx->ldim, &p->pmb);
    csound->AuxAlloc(csound, sizeof(float) * (p->ctx->ldim * p->ctx->maxlen_seq), &p->tmb);

    /* empty cache forces embedding on the first perf pass */
    csound->AuxAlloc(csound, 1, &p->last_text);
    ((char *) p->last_text.auxp)[0] = '\0';

    csound->AuxAlloc(csound, sizeof(Score) * p->top_k_neighs, &p->mheap);

    return OK;
}

int sem_space_query_perf(CSOUND *csound, SEM_SPACE_QUERY *p) {
    SEMSYS_SPACE *spc = p->spc;

    MYFLT *ids = (MYFLT *) p->ids.auxp;
    MYFLT *att = (MYFLT *) p->att.auxp;
    MYFLT *pool_embed_query = (MYFLT *) p->pmb.auxp;
    MYFLT *tmb = (MYFLT *) p->tmb.auxp;

    const char *sentence = p->query->data;

    /* self-gate: skip the model when the query is unchanged */
    if (strcmp(sentence, (char *) p->last_text.auxp) == 0) {
        return OK;
    }
    size_t tlen = strlen(sentence) + 1;
    if (tlen > (size_t) p->last_text.size) {
        csound->AuxAlloc(csound, tlen, &p->last_text);
    }
    strcpy((char *) p->last_text.auxp, sentence);

    int err = embed_sentence(ids, att, pool_embed_query, tmb, p->ctx, sentence);
    if (err == NOTOK) {
        return csound->PerfError(csound, &(p->h), "[semspacequery] onnx model error");
    };

    float *query = (float *) p->query_buf.auxp;
    for (uint32_t i = 0; i < spc->ldim; i++) {
        query[i] = (float) pool_embed_query[i];
    }

    normalize(query, spc->ldim);

    if (spc->count == 0) { return OK; }

    Score *mheap = (Score *) p->mheap.auxp;
    int mheap_size = 0;
    for (uint64_t i = 0; i < spc->count; i++) {
        size_t offset = i * spc->ldim;
        float curr_score = dot(query, spc->vectors + offset, spc->ldim);
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

    MYFLT *neighs = (MYFLT *) p->neighs->data;
    MYFLT *out_scores = (MYFLT *) p->scores->data;

    for (int i = 0; i < p->top_k_neighs; i++) {
        Score neigh = mheap[i];
        out_scores[i] = (MYFLT) neigh.score;
        float *vec = spc->vectors + neigh.ndx * spc->ldim;
        for (uint32_t j = 0; j < spc->ldim; j++) {
            neighs[i * spc->ldim + j] = (MYFLT) vec[j];
        }
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



#define S(s) sizeof(s)

static OENTRY localops[] = {
    { "semload",          S(SEM_INIT),             0, "i",         "iS",   (SUBR)sem_init,                   NULL,                       (SUBR)sem_deinit,       NULL, 0 },
    { "semload.tok",      S(SEM_INIT),             0, "i",         "iSS",  (SUBR)sem_init,                   NULL,                       (SUBR)sem_deinit,       NULL, 0 },
    { "semdim",           S(SEM_DIM),              0, "i",         "i",    (SUBR)sem_dim,                    NULL,                       NULL,                   NULL, 0 },
    { "semembed",         S(SEM_EMBED),            0, "k[]k[][]k", "iS",   (SUBR)sem_embed_init,             (SUBR)sem_embed_perf,       NULL,                   NULL, 0 },
    { "semspace",         S(SEM_SPACE_INIT),       0, "i",         "i",    (SUBR)sem_space_init,             NULL,                       (SUBR)sem_space_deinit, NULL, 0 },
    { "semspace.f",       S(SEM_SPACE_INIT),       0, "i",         "iS",   (SUBR)sem_space_init,             NULL,                       (SUBR)sem_space_deinit, NULL, 0 },
    { "semspace.vs",      S(SEM_SPACE_INIT_VS),    0, "i",         "iS[]", (SUBR)sem_space_init_vs,          NULL,                       (SUBR)sem_space_deinit, NULL, 0 },
    { "semspacebuild",    S(SEM_SPACE_BUILD),      0, "",          "iSS",  (SUBR)sem_space_build,            NULL,                       NULL,                   NULL, 0 },
    { "semspaceadd",      S(SEM_SPACE_ADD),        0, "",          "iS",   (SUBR)sem_space_add_init,         (SUBR)sem_space_add,        NULL,                   NULL, 0 },
    { "semspacesave",     S(SEM_SPACE_SAVE),       0, "",          "iS",   (SUBR)sem_space_save,             NULL,                       NULL,                   NULL, 0 },
    { "semspacesave.k",   S(SEM_SPACE_SAVE),       0, "",          "iSk",  (SUBR)sem_space_save_kset,        (SUBR)sem_space_save_kperf, NULL,                   NULL, 0 },
    { "semspacequery",    S(SEM_SPACE_QUERY),      0, "k[][]k[]",  "iSi",  (SUBR)sem_space_query_init,       (SUBR)sem_space_query_perf, NULL,                   NULL, 0 }
};

LINKAGE
