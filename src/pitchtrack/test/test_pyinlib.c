/*
 * test_pyinlib.c – correctness baseline for the pYIN library (pyinlib.c).
 *
 * Purpose: pin down observable behaviour BEFORE any performance refactor,
 * so later optimisation patches can be validated against it.  Covers:
 *   - config validation (valid default accepted, bad configs rejected)
 *   - silence  -> unvoiced, pitch 0
 *   - clean sine 440 Hz -> voiced, pitch within tolerance
 *   - clean sine 110 Hz -> voiced, pitch within tolerance (large lags)
 *   - determinism: two fresh contexts on identical input give identical output
 *   - edge cases: single block yields no result, destroy(NULL) is safe,
 *     result fields stay in range
 *
 * Test signals use a sine with +-1% vibrato at 5 Hz rather than a mathematically
 * pure tone.  Rationale, verified empirically: the tracker picks the global
 * maximum of the per-lag voiced probability (no first-dip preference), so on
 * a perfectly periodic tone every multiple of the true period dips equally
 * deeply and the decoded pitch sits on a knife-edge between subharmonics
 * (e.g. a pure 440 Hz sine reports 110 Hz, and float rounding alone can flip
 * the answer).  A touch of vibrato breaks the ties the way real signals do;
 * with it, tracking is stable with comfortable margins.  That pure-tone
 * behaviour is a known accuracy limitation, not something this test pins.
 *
 * Signal tests run under BOTH difference-function backends
 * (cfg.diff_use_fft true/false); the backends must additionally agree
 * frame-by-frame (same voicing, pitch within 0.05 Hz), which is far above
 * measured float-level divergence (~3e-3 Hz max) but far below any musical
 * difference such as an octave flip.
 *
 * Build & run (from src/pitchtrack/test/):
 *   gcc -O2 -Wall -Wextra -o test_pyinlib test_pyinlib.c ../src/pyinlib.c \
 *       ../src/kiss_fft.c ../src/kiss_fftr.c -lm
 *   ./test_pyinlib
 *
 * Exit code is 0 when all checks pass, 1 otherwise.  All signals are
 * synthesised deterministically; no input files or libraries required.
 */

#include "../src/pyinlib.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int n_checks = 0;
static int n_failed = 0;

#define CHECK(cond, ...) do {                                   \
        n_checks++;                                             \
        if (!(cond)) {                                          \
            n_failed++;                                         \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);          \
            printf(__VA_ARGS__);                                \
            printf("\n");                                       \
        }                                                       \
    } while (0)

/* Feed `n_blocks` blocks of `block_size` samples from `sig` (wrapping if
 * needed) through ctx, collecting up to `max_res` results. Returns the
 * number of results produced. */
static int feed(PYINContext *ctx, const float *sig, int n_sig,
                int block_size, int n_blocks,
                PYINResult *out, int max_res)
{
    float *block = (float *)malloc((size_t)block_size * sizeof(float));
    int n_res = 0;
    int pos = 0;
    PYINResult r;
    if (!block) return 0;
    for (int b = 0; b < n_blocks; b++) {
        for (int i = 0; i < block_size; i++)
            block[i] = sig[(pos + i) % n_sig];
        pos = (pos + block_size) % n_sig;
        if (pyin_process_block(ctx, block, &r) && n_res < max_res)
            out[n_res++] = r;
    }
    free(block);
    return n_res;
}

/* Sine with +-1% vibrato at 5 Hz (see header comment for why). Phase is
 * accumulated sample by sample; fully deterministic. */
static void make_vibrato_tone(float *dst, int n, float freq, float sr,
                              float amp)
{
    float phase = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr;
        float f = freq * (1.0f + 0.01f * sinf(2.0f * (float)M_PI * 5.0f * t));
        phase += 2.0f * (float)M_PI * f / sr;
        dst[i] = amp * sinf(phase);
    }
}

static float mean_pitch(const PYINResult *res, int n)
{
    double acc = 0.0;
    for (int i = 0; i < n; i++)
        acc += res[i].pitch_hz;
    return n > 0 ? (float)(acc / n) : 0.0f;
}

static float voiced_ratio(const PYINResult *res, int n)
{
    int v = 0;
    for (int i = 0; i < n; i++)
        if (res[i].voiced) v++;
    return n > 0 ? (float)v / (float)n : 0.0f;
}

/* Skip warmup frames (ring fill + Viterbi depth) before measuring. */
#define WARMUP 30

int main(void)
{
    const float sr = 44100.0f;
    const int block_size = 64;
    const int hop_size = 512;
    const int n_hops = 70;
    const int n_blocks = n_hops * hop_size / block_size; /* 560 */
    const int sig_len = n_blocks * block_size;

    /* ---- 1. config validation ---- */
    {
        PYINConfig good = pyin_config_default();
        PYINContext *ctx = pyin_create(good, NULL, NULL, NULL);
        CHECK(ctx != NULL, "default config should create a context");
        pyin_destroy(ctx);

        PYINConfig bad = good;
        bad.block_size = 0;
        CHECK(pyin_create(bad, NULL, NULL, NULL) == NULL,
              "block_size=0 should be rejected");

        bad = good;
        bad.f0_min = bad.f0_max;
        CHECK(pyin_create(bad, NULL, NULL, NULL) == NULL,
              "f0_min==f0_max should be rejected");

        bad = good;
        bad.hop_size = bad.frame_size + 1;
        CHECK(pyin_create(bad, NULL, NULL, NULL) == NULL,
              "hop_size>frame_size should be rejected");

        bad = good;
        bad.beta_b = 0.0f;
        CHECK(pyin_create(bad, NULL, NULL, NULL) == NULL,
              "beta_b=0 should be rejected");

        pyin_destroy(NULL); /* must not crash */
    }

    /* ---- 2. single block yields no result yet ---- */
    {
        PYINConfig cfg = pyin_config_default();
        PYINContext *ctx = pyin_create(cfg, NULL, NULL, NULL);
        float *blk = (float *)calloc((size_t)block_size, sizeof(float));
        PYINResult r;
        memset(&r, 0xAA, sizeof(r));
        CHECK(pyin_process_block(ctx, blk, &r) == false,
              "first block should need more samples");
        free(blk);
        pyin_destroy(ctx);
    }

    /* ---- 3. silence -> unvoiced (both backends; gate path is shared) ---- */
    for (int be = 0; be < 2; be++) {
        PYINConfig cfg = pyin_config_default();
        cfg.diff_use_fft = (be == 1);
        PYINContext *ctx = pyin_create(cfg, NULL, NULL, NULL);
        CHECK(ctx != NULL, "create with diff_use_fft=%d", be);
        float *sig = (float *)calloc((size_t)sig_len, sizeof(float));
        PYINResult res[128];
        int n = feed(ctx, sig, sig_len, block_size, n_blocks, res, 128);
        CHECK(n > WARMUP, "expected frame results for silence, got %d", n);
        for (int i = WARMUP; i < n; i++) {
            CHECK(!res[i].voiced, "silence frame %d should be unvoiced", i);
            CHECK(res[i].pitch_hz == 0.0f,
                  "silence frame %d pitch should be 0", i);
            CHECK(res[i].confidence == 0.0f,
                  "silence frame %d confidence should be 0", i);
        }
        free(sig);
        pyin_destroy(ctx);
    }

    /* ---- 4-6. vibrato tones at 110/220/440 Hz ----
     * Per-frame tolerance 3% comfortably contains the +-1% vibrato plus
     * tracking lag; the mean must sit within 1%. */
    {
        const float f0s[] = { 110.0f, 220.0f, 440.0f };
        for (int fi = 0; fi < 3; fi++)
        for (int be = 0; be < 2; be++) {
            float f0 = f0s[fi];
            PYINConfig cfg = pyin_config_default();
            cfg.sample_rate = sr;
            cfg.diff_use_fft = (be == 1);
            PYINContext *ctx = pyin_create(cfg, NULL, NULL, NULL);
            CHECK(ctx != NULL, "create f0=%f be=%d", f0, be);
            float *sig = (float *)malloc((size_t)sig_len * sizeof(float));
            PYINResult res[128];
            make_vibrato_tone(sig, sig_len, f0, sr, 0.5f);
            int n = feed(ctx, sig, sig_len, block_size, n_blocks, res, 128);
            CHECK(n > WARMUP, "expected frame results for %fHz, got %d",
                  f0, n);
            float vr = voiced_ratio(res + WARMUP, n - WARMUP);
            float mp = mean_pitch(res + WARMUP, n - WARMUP);
            CHECK(vr > 0.95f, "%fHz voiced ratio %f should exceed 0.95",
                  f0, vr);
            CHECK(fabsf(mp - f0) / f0 < 0.01f,
                  "%fHz mean pitch %f should be within 1%%", f0, mp);
            for (int i = WARMUP; i < n; i++) {
                CHECK(res[i].voiced, "%fHz frame %d should be voiced",
                      f0, i);
                CHECK(fabsf(res[i].pitch_hz - f0) / f0 < 0.03f,
                      "%fHz frame %d pitch %f should be within 3%%",
                      f0, i, res[i].pitch_hz);
                CHECK(res[i].confidence >= 0.0f &&
                      res[i].confidence <= 1.0f,
                      "confidence %f out of range", res[i].confidence);
                CHECK(res[i].pitch_hz >= 0.0f, "negative pitch");
            }
            free(sig);
            pyin_destroy(ctx);
        }
    }

    /* ---- 7. determinism (both backends, exact) ---- */
    for (int be = 0; be < 2; be++) {
        PYINConfig cfg = pyin_config_default();
        cfg.sample_rate = sr;
        cfg.diff_use_fft = (be == 1);
        float *sig = (float *)malloc((size_t)sig_len * sizeof(float));
        PYINResult a[128], b[128];
        make_vibrato_tone(sig, sig_len, 330.0f, sr, 0.5f);
        PYINContext *c1 = pyin_create(cfg, NULL, NULL, NULL);
        PYINContext *c2 = pyin_create(cfg, NULL, NULL, NULL);
        int na = feed(c1, sig, sig_len, block_size, n_blocks, a, 128);
        int nb = feed(c2, sig, sig_len, block_size, n_blocks, b, 128);
        CHECK(na == nb, "be=%d determinism run length %d vs %d",
              be, na, nb);
        for (int i = 0; i < na && i < 128; i++) {
            CHECK(a[i].pitch_hz == b[i].pitch_hz &&
                  a[i].confidence == b[i].confidence &&
                  a[i].voiced == b[i].voiced,
                  "be=%d frame %d differs between identical runs",
                  be, i);
        }
        free(sig);
        pyin_destroy(c1);
        pyin_destroy(c2);
    }

    /* ---- 8. cross-backend agreement ----
     * FFT and direct paths compute the same quantity up to float rounding
     * (measured max |dpitch| ~3e-3 Hz).  Same voicing plus tight absolute
     * tolerances catch any musical divergence (e.g. octave flips). */
    {
        const float f0s[] = { 110.0f, 220.0f, 330.0f, 440.0f };
        for (int fi = 0; fi < 4; fi++) {
            float f0 = f0s[fi];
            PYINConfig fa = pyin_config_default();
            PYINConfig fb = pyin_config_default();
            fa.sample_rate = fb.sample_rate = sr;
            fa.diff_use_fft = true;
            fb.diff_use_fft = false;
            float *sig = (float *)malloc((size_t)sig_len * sizeof(float));
            PYINResult a[128], b[128];
            make_vibrato_tone(sig, sig_len, f0, sr, 0.5f);
            PYINContext *ca = pyin_create(fa, NULL, NULL, NULL);
            PYINContext *cb = pyin_create(fb, NULL, NULL, NULL);
            int na = feed(ca, sig, sig_len, block_size, n_blocks, a, 128);
            int nb = feed(cb, sig, sig_len, block_size, n_blocks, b, 128);
            CHECK(na == nb, "f0=%f backend run length %d vs %d",
                  f0, na, nb);
            for (int i = 0; i < na && i < 128; i++) {
                CHECK(a[i].voiced == b[i].voiced,
                      "f0=%f frame %d voicing differs", f0, i);
                CHECK(fabsf(a[i].pitch_hz - b[i].pitch_hz) < 0.05f,
                      "f0=%f frame %d pitch differs: %f vs %f",
                      f0, i, a[i].pitch_hz, b[i].pitch_hz);
                CHECK(fabsf(a[i].confidence - b[i].confidence) < 1e-3f,
                      "f0=%f frame %d confidence differs", f0, i);
            }
            free(sig);
            pyin_destroy(ca);
            pyin_destroy(cb);
        }
    }

    if (n_failed == 0)
        printf("PASS: all %d checks passed\n", n_checks);
    else
        printf("FAIL: %d of %d checks failed\n", n_failed, n_checks);
    return n_failed == 0 ? 0 : 1;
}
