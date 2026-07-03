/*
    semsys.h:

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

#ifndef SEMSYS_H
#define SEMSYS_H

#include "csdl.h"
#include "csound.h"
#include "sysdep.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <onnxruntime_c_api.h>

#ifndef CSOUNDAPI7
#error "semsys requires Csound API 7"
#endif

#define CACHE_HEADER_TAG 0x65737063 // <espc>
#define CACHE_DATA_TAG 0x64617461u  // <data>

#define INITIAL_VCAPACITY 100

#define CTX_TO_FLT(ctx) (MYFLT) (uintptr_t) (ctx)
#define FLT_TO_CTX(p) (SEMSYS *) (uintptr_t) (*p->handle)
#define SPC_TO_FLT(spc) (MYFLT)(uintptr_t) (spc)
#define FLT_TO_SPC(p) (SEMSYS_SPACE *) (uintptr_t) (*p->s_handle)
#define FLT_TO_STT(stt) (SEMSYS_STT *) (uintptr_t) (*p->handle)
#define STT_TO_FLT(stt) (MYFLT) (uintptr_t) (stt)
#define CACHE_INDEX(spc, ndx) sizeof(CACHE_HEADER) + (ndx) * (spc)->ldim * sizeof(float)

#define TOKWIN(maxlen_seq) (int) ((maxlen_seq) * 0.6) > 1 ? (int) ((maxlen_seq) * 0.6) : 1
#define TOKSTRIDE(wsize) ((wsize) - (int) (0.15 * (wsize) + 0.5)) > 1 ? ((wsize) - (int) (0.15 * (wsize) + 0.5)) : 1

#define STT_IDLE_TICKS 20
#define STT_WAIT_MS 100
#define STT_DEFAULT_QUEUE_CAP 256
#define STT_MAX_QUEUE_CAP 512
#define STT_LIVE_HARD_MAX_SEC 30.0
// offline file/array/func segmentation: split long PCM into ~28s chunks, snapping
// the cut to the quietest 20ms frame in the 2s before the target end (hard cap 30s)
#define STT_FILE_CHUNK_TARGET_SEC 28.0
#define STT_FILE_CHUNK_SEARCH_SEC 2.0
#define STT_LIVE_VAD_FRAME_SEC 0.02
#define STT_LIVE_VAD_RMS 0.010
#define STT_LIVE_VAD_PEAK 0.035
#define STT_LIVE_MIN_SPEECH_SEC 0.80
#define STT_LIVE_TRAILING_SILENCE_SEC 0.45
#define STT_LIVE_PAD_BEFORE_SEC 0.25
#define STT_LIVE_PAD_AFTER_SEC 0.35

#define STT_DEBUG_ENABLED                       \
    (getenv("SEMSYS_STT_DEBUG") != NULL   &&    \
    getenv("SEMSYS_STT_DEBUG")[0] != '\0' &&    \
    getenv("SEMSYS_STT_DEBUG")[0] != '0')       \


#define CHECK_PTR_CTX(csound, ctx, ctx_string)                                  \
    do {                                                                        \
        if (ctx == NULL) {                                                      \
            return csound->InitError(csound, "[%s] Null context", ctx_string);  \
        }                                                                       \
    } while(0)

#define ONNX_CHECK_INIT(api, expr)                                \
    do {                                                          \
        OrtStatus *status = (expr);                               \
        if (status != NULL) {                                     \
            const char *msg = (api)->GetErrorMessage(status);     \
            int ret = csound->InitError(                          \
                csound,                                           \
                "onnxruntime error: %s",                          \
                msg                                               \
            );                                                    \
            (api)->ReleaseStatus(status);                         \
            return ret;                                           \
        }                                                         \
    } while (0)


#define ONNX_CHECK_PERF(api, expr)                                \
    do {                                                          \
        OrtStatus *status = (expr);                               \
        if (status != NULL) {                                     \
            const char *msg = (api)->GetErrorMessage(status);     \
            int ret = csound->PerfError(                          \
                csound,                                           \
                &(p->h),                                          \
                "onnxruntime error: %s",                          \
                msg                                               \
            );                                                    \
            (api)->ReleaseStatus(status);                         \
            return ret;                                           \
        }                                                         \
    } while (0)

#define ONNX_CHECK(api, expr)                                     \
    do {                                                          \
        OrtStatus *status = (expr);                               \
        if (status != NULL) {                                     \
            (api)->ReleaseStatus(status);                         \
            return 1;                                             \
        }                                                         \
    } while (0)

/* like ONNX_CHECK but jumps to a local `fail:` label so the caller can
   release any tensors it already created before returning the error */
#define ONNX_CHECK_GOTO(api, expr)                                \
    do {                                                          \
        OrtStatus *_st = (expr);                                  \
        if (_st != NULL) {                                        \
            (api)->ReleaseStatus(_st);                            \
            goto fail;                                            \
        }                                                         \
    } while (0)

/* like ONNX_CHECK_GOTO but also records an init error in `ret` (caller must
   declare `int ret` and a `fail:` label that returns `ret`) */
#define ONNX_CHECK_STT(api, expr)                                 \
    do {                                                          \
        OrtStatus *_st = (expr);                                  \
        if (_st != NULL) {                                        \
            ret = csound->InitError(                              \
                csound, "[semstt] onnxruntime error: %s",         \
                (api)->GetErrorMessage(_st));                     \
            (api)->ReleaseStatus(_st);                            \
            goto fail;                                            \
        }                                                         \
    } while (0)

/* ORT check for stt_run: record the message into errbuf and bail (sets ret=NOTOK).
   needs `api`, `errbuf`, `errcap`, `ret` in scope. */
#define ORT_CK(expr)                                                               \
do {                                                                               \
    OrtStatus *_s = (expr);                                                        \
    if (_s != NULL) {                                                              \
            snprintf(errbuf, errcap, "onnxruntime: %s", api->GetErrorMessage(_s)); \
            api->ReleaseStatus(_s);                                                \
            ret = NOTOK;                                                           \
            goto fail;                                                             \
        }                                                                          \
    } while (0)


// core
typedef struct {
    char model_path[1024];
    const OrtApi *api;
    OrtEnv *env;
    OrtSessionOptions *emb_session_options;
    OrtSession *emb_session;
    uint32_t ldim;
    int32_t maxlen_seq; // text token cap; <= 0 (e.g. -1) means "full, no cap" (audio embedder)
    int is_audio;       // 1 if input 0 is a float waveform (audio model), 0 if a STRING (text)
} SEMSYS;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *handle;
    // inputs
    MYFLT *maxlen_seq; // depends on the onnx model (see max position embeedding for embedding model and truncation for tokenizer)
    STRINGDAT *model_dir;
} SEM_INIT;

typedef struct {
    OPDS h;
    //output
    MYFLT *out_dim;
    //input
    MYFLT *handle;
} SEM_DIM;

typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *pool_embed;
    MYFLT *gate;
    //inputs
    MYFLT *handle;
    STRINGDAT *text;
    // private state
    SEMSYS *ctx;
    AUXCH last_text;
} SEM_EMBED_TEXT;

// i-rate form of semembed: 1 output (pooled embedding), no gate / no change-cache
typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *pool_embed;
    // inputs
    MYFLT *handle;
    STRINGDAT *text;
    // private state
    SEMSYS *ctx;
} SEM_EMBED_TEXT_I;

typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *pool_embed;
    // inputs
    MYFLT *handle;
    STRINGDAT *file_path;
    // private state
    SEMSYS *ctx;
} SEM_EMBED_TEXT_FILE_I;

typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *pool_embed; // 1D [ldim], refreshed when a fresh embedding is ready
    MYFLT *gate;          // 1 on the k-pass a fresh embedding is published, else 0
    // inputs
    MYFLT *handle;
    MYFLT *asig;          // a-rate audio in (engine sr, mono)
    MYFLT *iwindow;       // window length in seconds (optional, default 10, min 1)
    // private state
    SEMSYS *ctx;
    AUXCH buf;            // MYFLT engine-sr accumulation (perf-thread only)
    size_t target;       // window length in samples (engine sr)
    size_t len;          // samples buffered so far
    uint32_t esr;        // engine sr
    // async worker: resample + inference run off the audio thread (like STT) to avoid xruns
    CSOUND *csound;
    void *thread;
    void *mutex;         // guards job_* and result_*
    void *job_lock;      // wakes the worker
    volatile int stop;
    int worker_done;
    MYFLT *job_buf;      // handoff window (malloc, target samples)
    size_t job_len;
    int job_pending;     // 1 = a window is waiting for the worker (latest wins)
    MYFLT *result;       // last embedding (malloc, ldim)
    int result_ready;    // 1 = fresh embedding to publish
} SEM_EMBED_AUDIO;

typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *pool_embed; // 2D [nchunks, ldim]
    // inputs
    MYFLT *handle;
    MYFLT *func_number;
    // private state
    SEMSYS *ctx;
} SEM_EMBED_AUDIO_FUNC_I;

typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *pool_embed; // 2D [nchunks, ldim]
    // inputs
    MYFLT *handle;
    STRINGDAT *file_path;
    // private state
    SEMSYS *ctx;
} SEM_EMBED_AUDIO_FILE_I;


// --- SEM_SPACE ---

//  cache file (.espc)
//  [HEADER]
//  <espc>     [4 byte]
//  embed_dim  [4 byte]
//  count      [8 byte]
//
//  [DATA]
//  <data>       [4 byte]
//  n x each vec [4 byte]

typedef struct {
    uint32_t head_tag;
    uint32_t ldim;
    uint64_t count;
    uint32_t data_tag;
} CACHE_HEADER;

// space core. the space is a pure vector store: it fixes ldim from the handle passed to
// semspace and does NOT embed. the embedding model is chosen by the user per operation
// (semspaceaddtxt/addaudio/querytxt/queryaudio each take a model handle), so text and audio
// vectors of matching dim can share one space / one .espc.
typedef struct {
    uint32_t ldim;
    uint64_t count;
    SEMSYS *ctx;       // borrowed model bound at init; ldim anchor only (add/query bring their own)
    float *vectors;
    uint64_t capacity;
} SEMSYS_SPACE;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *s_handle; // space handle
    // inputs
    MYFLT *handle; // sem_emb_handle
    STRINGDAT *cache_file; // optional: a .espc file or a directory of .espc files to load into RAM
} SEM_SPACE_INIT;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *s_handle; // space handle
    // inputs
    MYFLT *handle; // sem_emb_handle
    ARRAYDAT *paths; // S[] of .espc file paths to merge into RAM
} SEM_SPACE_INIT_VS;


typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle; // sem_emb_handle
    STRINGDAT *file_name;
    STRINGDAT *source;
    // private
    MYFLT *pmb;
    float *pool;
} SEM_SPACE_BUILD;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *s_handle;   // space handle
    MYFLT *handle;     // model handle (text for addtxt, audio for addaudio); chosen per-op
    STRINGDAT *sentence; // text (addtxt) or audio file path (addaudio)
    MYFLT *ktrig; // k-form only: add on rising edge
    // private state
    SEMSYS_SPACE *spc;
    AUXCH last_text; // cached last-added text/path; self-gate skips re-embedding unchanged input
    AUXCH vec_scratch; // float[ldim] scratch for chunk_and_embed (text form)
    SEMSYS *ctx;     // resolved from *handle at init
    AUXCH pmb;
    MYFLT prev_trig;
} SEM_SPACE_ADD;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *s_handle; // space handle
    STRINGDAT *fname;
    MYFLT *ktrig; // k-form only: save on rising edge
    // private state
    SEMSYS_SPACE *spc;
    MYFLT prev_trig;
} SEM_SPACE_SAVE;

typedef struct {
    uint64_t ndx;
    float score;
} Score;

// async query context: a per-instance worker embeds the query + runs the top-k search off
// the audio thread (large-space search and the ONNX embed both block otherwise). latest-wins
// single slot; the perf pass publishes the freshest result into the output arrays. shared by the
// perf-time (.k) query forms.
typedef struct {
    CSOUND *csound;
    void *thread;
    void *mutex;         // guards job_* and result_*
    void *job_lock;      // wakes the worker
    volatile int stop;
    int worker_done;
    // shared read-only, set at init
    SEMSYS_SPACE *spc;
    SEMSYS *ctx;
    int top_k_neighs;
    int kind;            // 0 text, 1 audio file path, 2 ftable pcm snapshot
    // job (perf -> worker), latest-wins
    int job_pending;
    char *job_str;       // text/path (kinds 0/1), malloc'd
    MYFLT *job_pcm;      // pcm snapshot (kind 2), malloc'd
    size_t job_n;
    uint32_t job_sr;
    // worker-private scratch (alloc at init)
    MYFLT *pmb;          // ldim
    float *pool;         // ldim
    float *qvec;         // ldim centroid
    Score *mheap;        // top_k
    // result (worker -> perf), mutex-guarded
    int result_ready;
    int result_error;
    uint64_t submitted_id;
    uint64_t result_id;
    Score *res;          // top_k, sorted descending
    int res_count;
} QUERY_ASYNC;

typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *neighs; // need to return also string
    ARRAYDAT *scores;
    // inputs
    MYFLT *s_handle;   // space handle
    MYFLT *handle;     // model handle (text for querytxt, audio for queryaudio); chosen per-op
    STRINGDAT *query;  // query text (querytxt) or audio file path (queryaudio)
    MYFLT *top_k;
    // private
    SEMSYS_SPACE *spc;
    int top_k_neighs;
    AUXCH query_buf; // float[ldim] normalized query
    AUXCH last_text; // cached prev sentence for change detection
    SEMSYS *ctx;
    AUXCH pmb;
    AUXCH mheap;
    QUERY_ASYNC async;
} SEM_SPACE_QUERY;

typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *neighs;
    ARRAYDAT *scores;
    MYFLT *gate;      // 1 on the k-pass a fresh query result is published, else 0
    // inputs
    MYFLT *s_handle;
    MYFLT *handle;
    STRINGDAT *query;
    MYFLT *top_k;
    // private
    SEMSYS_SPACE *spc;
    int top_k_neighs;
    AUXCH query_buf;
    AUXCH last_text;
    SEMSYS *ctx;
    AUXCH pmb;
    AUXCH mheap;
    QUERY_ASYNC async;
} SEM_SPACE_QUERY_K;

// real-time audio query from an ftable (samples at engine sr), not a file. iminsec sets the
// minimum audio duration to run a query, so short/partial live buffers can be gated out.
typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *neighs;
    ARRAYDAT *scores;
    // inputs
    MYFLT *s_handle;
    MYFLT *handle;    // audio model handle
    MYFLT *ftable;    // ftable of mono audio samples (engine sr)
    MYFLT *top_k;
    MYFLT *iminsec;   // optional: min duration (s) to run a query; 0 = no minimum
    // private
    SEMSYS_SPACE *spc;
    int top_k_neighs;
    AUXCH query_buf;
    SEMSYS *ctx;
    AUXCH mheap;
} SEM_SPACE_QUERY_FT;

// k-rate form: same, plus a trigger (query on rising edge). ktrig comes before the optional
// iminsec so the field order matches the argument order.
typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *neighs;
    ARRAYDAT *scores;
    MYFLT *gate;      // 1 on the k-pass a fresh query result is published, else 0
    // inputs
    MYFLT *s_handle;
    MYFLT *handle;    // audio model handle
    MYFLT *ftable;
    MYFLT *top_k;
    MYFLT *ktrig;     // query on rising edge
    MYFLT *iminsec;   // optional: min duration (s) to run a query; 0 = no minimum
    // private
    SEMSYS_SPACE *spc;
    int top_k_neighs;
    AUXCH query_buf;
    SEMSYS *ctx;
    AUXCH mheap;
    MYFLT prev_trig;
    QUERY_ASYNC async;
} SEM_SPACE_QUERY_FT_K;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *s_handle; // space handle
    MYFLT *trig;
    // private
    SEMSYS_SPACE *spc;
    MYFLT last_trig;
} SEM_SPACE_CLEAR_K;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *s_handle; // space handle
} SEM_SPACE_CLEAR_I;

// SEMSYS STT

// core
// one queued audio job: one or more malloc'd encoded byte buffers (file blob or
// in-RAM WAV). long PCM is pre-split into <=30s segments (Whisper's fixed window);
// the worker transcribes each chunk and joins the texts into a single result.
typedef struct {
    uint8_t **chunks;   // nchunks malloc'd byte buffers
    size_t *sizes;      // byte size of each chunk
    int nchunks;
    uint64_t id;
} STT_JOB;

typedef struct {
    char model_path[1024];
    const OrtApi *api;
    OrtEnv *env;
    OrtSessionOptions *mod_session_options;
    OrtSession *mod_session;
    int32_t max_length;
    // async worker
    CSOUND *csound;     // for the worker's thread-API calls
    void *thread;       // background worker (one per handle)
    void *mutex;        // protects both queues + err
    void *job_lock;     // wakes the worker
    volatile int stop;  // deinit -> worker exits
    int worker_done;    // worker returned; join handle before restarting/freeing
    // fixed FIFO of pending jobs (ring buffer, capacity qcap)
    STT_JOB *jobs;
    int qcap;
    int qhead;          // oldest job index
    int qcount;         // jobs queued
    uint64_t next_job_id;
    // FIFO of finished transcriptions (ring, same capacity)
    char **results;     // malloc'd texts, consumed in order by semsttresult
    uint64_t *result_ids;
    int rhead;          // oldest result index
    int rcount;         // results waiting to be read
    char err[256];      // last error message
} SEMSYS_STT;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *handle;
    // inputs
    STRINGDAT *model_dir;
    MYFLT *max_length;
    MYFLT *queue_depth;   // FIFO capacity (jobs + results); 0 -> default
} SEM_STT_INIT;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    STRINGDAT *audio_speech_fpath;
} SEM_STT_SUBMIT_AUDIO_FILE;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    ARRAYDAT *audio_speech_arr;
} SEM_STT_SUBMIT_AUDIO_ARRAY;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    MYFLT *ftable_num;
} SEM_STT_SUBMIT_AUDIO_FUNC;

// live a-rate capture: accumulate asig across k-blocks, submit a window every imaxdur
// seconds (auto) and/or on the rising edge of the optional ktrig
typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    MYFLT *asig;      // a-rate audio in
    MYFLT *imaxdur;   // window length, seconds (buffer size; auto-submit when full)
    MYFLT *ktrig;     // optional: also submit on rising edge (default 0 -> auto only)
    // state
    SEMSYS_STT *ctx;
    AUXCH buf;         // accumulation buffer (MYFLT, engine sr, mono)
    size_t target;     // preferred window length in samples
    size_t cap;        // hard capacity in samples
    size_t next_check; // next automatic VAD boundary check
    size_t len;        // samples written so far
    MYFLT prev_trig;
} SEM_STT_SUBMIT_LIVE;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *is_ready; // bool: 1 when a fresh transcription is ready
    // inputs
    MYFLT *handle;
} SEM_STT_READY;

typedef struct {
    OPDS h;
    // outputs
    STRINGDAT *result;
    MYFLT *text_length;
    // inputs
    MYFLT *handle;
} SEM_STT_RESULT;

typedef struct {
    size_t first_voice;
    size_t last_voice_end;
    size_t voiced_samples;
    double rms;
    double peak;
    int has_voice;
} STT_LIVE_ANALYSIS;


// SEMSYS EMBED
int sem_init(CSOUND *csound, SEM_INIT *p);
int sem_dim(CSOUND *csound, SEM_DIM *p);
// text
int sem_embed_text_init(CSOUND *csound, SEM_EMBED_TEXT *p);
int sem_embed_text_perf(CSOUND *csound, SEM_EMBED_TEXT *p);
int sem_embed_text_i(CSOUND *csound, SEM_EMBED_TEXT_I *p);
int sem_embed_text_i_file(CSOUND *csound, SEM_EMBED_TEXT_FILE_I *p);
// audio
int sem_embed_audio_init(CSOUND *csound, SEM_EMBED_AUDIO *p);
int sem_embed_audio_perf(CSOUND *csound, SEM_EMBED_AUDIO *p);
int sem_embed_audio_deinit(CSOUND *csound, SEM_EMBED_AUDIO *p);
int sem_embed_audio_file_i(CSOUND *csound, SEM_EMBED_AUDIO_FILE_I *p);
int sem_embed_audio_func_i(CSOUND *csound, SEM_EMBED_AUDIO_FUNC_I *p);

// SEMSYS SPACE
int sem_space_init(CSOUND *csound, SEM_SPACE_INIT *p);
int sem_space_init_vs(CSOUND *csound, SEM_SPACE_INIT_VS *p);
int sem_space_clear_i(CSOUND *csound, SEM_SPACE_CLEAR_I *p);
int sem_space_clear_k(CSOUND *csound, SEM_SPACE_CLEAR_K *p);
// build is universal: it dispatches text vs audio on the model kind (semload-detected)
int sem_space_build(CSOUND *csound, SEM_SPACE_BUILD *p);
// text (add/query take the model handle per-op; audio forms differ only in the model used)
int sem_space_add_init(CSOUND *csound, SEM_SPACE_ADD *p);
int sem_space_add(CSOUND *csound, SEM_SPACE_ADD *p);
int sem_space_add_perf(CSOUND *csound, SEM_SPACE_ADD *p);
int sem_space_query_init(CSOUND *csound, SEM_SPACE_QUERY_K *p);
int sem_space_query_perf(CSOUND *csound, SEM_SPACE_QUERY_K *p);
int sem_space_query_i(CSOUND *csound, SEM_SPACE_QUERY *p);
int sem_space_query_deinit(CSOUND *csound, SEM_SPACE_QUERY_K *p);
// audio
int sem_space_add_audio(CSOUND *csound, SEM_SPACE_ADD *p);
int sem_space_add_audio_init(CSOUND *csound, SEM_SPACE_ADD *p);
int sem_space_add_audio_perf(CSOUND *csound, SEM_SPACE_ADD *p);
int sem_space_query_audio_init(CSOUND *csound, SEM_SPACE_QUERY_K *p);
int sem_space_query_audio_perf(CSOUND *csound, SEM_SPACE_QUERY_K *p);
int sem_space_query_audio_i(CSOUND *csound, SEM_SPACE_QUERY *p);
int sem_space_query_audio_deinit(CSOUND *csound, SEM_SPACE_QUERY_K *p);
// real-time audio query from an ftable
int sem_space_query_audio_ft_i(CSOUND *csound, SEM_SPACE_QUERY_FT *p);
int sem_space_query_audio_ft_init(CSOUND *csound, SEM_SPACE_QUERY_FT_K *p);
int sem_space_query_audio_ft_perf(CSOUND *csound, SEM_SPACE_QUERY_FT_K *p);
int sem_space_query_audio_ft_deinit(CSOUND *csound, SEM_SPACE_QUERY_FT_K *p);
// save (dim-agnostic, shared)
int sem_space_save(CSOUND *csound, SEM_SPACE_SAVE *p);
int sem_space_save_kset(CSOUND *csound, SEM_SPACE_SAVE *p);
int sem_space_save_kperf(CSOUND *csound, SEM_SPACE_SAVE *p);


// SEMSYS STT
int sem_stt_init(CSOUND *csound, SEM_STT_INIT *p);
int sem_stt_ready(CSOUND *csound, SEM_STT_READY *p);
int sem_stt_submit_audio_file(CSOUND *csound, SEM_STT_SUBMIT_AUDIO_FILE *p);
int sem_stt_submit_audio_arr(CSOUND *csound, SEM_STT_SUBMIT_AUDIO_ARRAY *p);
int sem_stt_submit_audio_func(CSOUND *csound, SEM_STT_SUBMIT_AUDIO_FUNC *p);
int sem_stt_submit_live_init(CSOUND *csound, SEM_STT_SUBMIT_LIVE *p);
int sem_stt_submit_live_perf(CSOUND *csound, SEM_STT_SUBMIT_LIVE *p);
int sem_stt_submit_live_deinit(CSOUND *csound, SEM_STT_SUBMIT_LIVE *p);
int sem_stt_result(CSOUND *csound, SEM_STT_RESULT *p);

#endif
