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
    uint32_t maxlen_seq;
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
} SEM_EMBED;

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
} SEM_EMBED_FILE_I;

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

// space core
typedef struct {
    uint32_t ldim;
    uint64_t count;
    SEMSYS *ctx; // borrowed model handle (owned by semload) for embedding sentences
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
    MYFLT *s_handle;
    STRINGDAT *sentence;
    MYFLT *ktrig; // k-form only: add on rising edge
    // private state
    SEMSYS_SPACE *spc;
    AUXCH last_text; // cached last-added text; self-gate skips re-embedding unchanged text
    AUXCH vec_scratch; // float[ldim] scratch for chunk_and_embed
    SEMSYS *ctx;
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

typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *neighs; // need to return also string
    ARRAYDAT *scores;
    // inputs
    MYFLT *s_handle;
    STRINGDAT *query;
    MYFLT *top_k;
    // private
    SEMSYS_SPACE *spc;
    int top_k_neighs;
    AUXCH query_buf; // float[ldim] normalized query
    AUXCH last_text; // cached prev sentence for change detection
    SEMSYS *ctx;
    AUXCH pmb;
    AUXCH mheap;
} SEM_SPACE_QUERY;


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
    AUXCH buf;        // accumulation buffer (MYFLT, engine sr, mono)
    size_t target;    // preferred window length in samples
    size_t cap;       // hard capacity in samples
    size_t next_check; // next automatic VAD boundary check
    size_t len;       // samples written so far
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
int sem_embed_init(CSOUND *csound, SEM_EMBED *p);
int sem_embed_perf(CSOUND *csound, SEM_EMBED *p);
int sem_embed_i(CSOUND *csound, SEM_EMBED_TEXT_I *p);
int sem_embed_i_file(CSOUND *csound, SEM_EMBED_FILE_I *p);

// SEMSYS SPACE
int sem_space_init(CSOUND *csound, SEM_SPACE_INIT *p);
int sem_space_init_vs(CSOUND *csound, SEM_SPACE_INIT_VS *p);
int sem_space_build(CSOUND *csound, SEM_SPACE_BUILD *p);
int sem_space_add_init(CSOUND *csound, SEM_SPACE_ADD *p);
int sem_space_add(CSOUND *csound, SEM_SPACE_ADD *p);
int sem_space_add_perf(CSOUND *csound, SEM_SPACE_ADD *p);
int sem_space_save(CSOUND *csound, SEM_SPACE_SAVE *p);
int sem_space_save_kset(CSOUND *csound, SEM_SPACE_SAVE *p);
int sem_space_save_kperf(CSOUND *csound, SEM_SPACE_SAVE *p);
int sem_space_query_init(CSOUND *csound, SEM_SPACE_QUERY *p);
int sem_space_query_perf(CSOUND *csound, SEM_SPACE_QUERY *p);
int sem_space_query_i(CSOUND *csound, SEM_SPACE_QUERY *p);

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
