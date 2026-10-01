/*
 * bench_pyinlib.c – throughput benchmark for pyinlib.c.
 *
 * Workload (deterministic): 30 s of mono 44.1 kHz audio mixing vibrato
 * tones (110/220/440 Hz), silence gaps, and shaped noise, fed through
 * pyin_process_block() in default-config blocks.  Reports wall time spent
 * inside pyin_process_block(), analysed frames, us/frame, and x-realtime.
 *
 * This measures the whole pipeline (diff + beta + HMM + decode).  Rerun
 * after optimisation patches to quantify the speedup; correctness is
 * covered separately by test_pyinlib.c.
 *
 * Build & run (from src/pitchtrack/test/):
 *   gcc -O2 -Wall -Wextra -o bench_pyinlib bench_pyinlib.c ../src/pyinlib.c -lm
 *   ./bench_pyinlib [seconds] [repeats]
 */
#include "../src/pyinlib.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static unsigned rng_state = 0x12345678u;
static float rndf(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return (float)(rng_state >> 9) * (1.0f / 8388608.0f) - 1.0f;
}

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(int argc, char **argv)
{
    double seconds = argc > 1 ? atof(argv[1]) : 30.0;
    int repeats = argc > 2 ? atoi(argv[2]) : 3;
    if (seconds <= 0.0) seconds = 30.0;
    if (repeats <= 0) repeats = 3;

    const float sr = 44100.0f;
    const int total = (int)(seconds * sr);
    float *sig = (float *)malloc((size_t)total * sizeof(float));
    if (!sig) { fprintf(stderr, "OOM\n"); return 1; }

    /* Deterministic mixed signal: vibrato tones, silence, noisy tone. */
    float phase = 0.0f;
    for (int i = 0; i < total; i++) {
        float t = (float)i / sr;
        int seg = (i / (int)sr) % 6;
        float s = 0.0f;
        if (seg == 0 || seg == 3) {
            float f0 = seg == 0 ? 130.0f : 330.0f;
            float f = f0 * (1.0f + 0.01f * sinf(2.0f * (float)M_PI * 5.0f * t));
            phase += 2.0f * (float)M_PI * f / sr;
            s = 0.5f * sinf(phase);
        } else if (seg == 1 || seg == 4) {
            s = 0.0f; /* silence -> energy-gate path */
        } else {
            float f = 220.0f * (1.0f + 0.01f * sinf(2.0f * (float)M_PI * 5.0f * t));
            phase += 2.0f * (float)M_PI * f / sr;
            s = 0.4f * sinf(phase) + 0.05f * rndf();
        }
        sig[i] = s;
    }

    PYINConfig cfg = pyin_config_default();
    cfg.sample_rate = sr;
    const int BS = cfg.block_size;

    double best = 1e300;
    long frames = 0;
    for (int r = 0; r < repeats; r++) {
        PYINContext *ctx = pyin_create(cfg, NULL, NULL, NULL);
        if (!ctx) { fprintf(stderr, "create failed\n"); return 1; }
        float *blk = (float *)malloc((size_t)BS * sizeof(float));
        PYINResult res;
        long nframes = 0;
        double t0 = now_sec();
        for (int pos = 0; pos + BS <= total; pos += BS) {
            for (int i = 0; i < BS; i++)
                blk[i] = sig[pos + i];
            if (pyin_process_block(ctx, blk, &res))
                nframes++;
        }
        double dt = now_sec() - t0;
        if (dt < best) best = dt;
        frames = nframes;
        free(blk);
        pyin_destroy(ctx);
    }

    printf("signal=%.1fs frames=%ld repeats=%d\n", seconds, frames, repeats);
    printf("best wall inside pyin_process_block: %.3f s\n", best);
    printf("us/frame: %.1f  x-realtime: %.1fx\n",
           best * 1e6 / (double)frames, seconds / best);
    free(sig);
    return 0;
}
