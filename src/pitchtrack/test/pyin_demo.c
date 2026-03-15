/*
 * pyin_demo.c – pYIN pitch tracker demo
 *
 * Usage:
 *   ./pyin_demo                  – synthesised speech-like signal
 *   ./pyin_demo  speech.wav      – mono or stereo PCM/float WAV file
 *
 * Supported WAV formats:
 *   PCM   16-bit  (format tag 1)
 *   PCM   24-bit  (format tag 1)
 *   PCM   32-bit  (format tag 1)
 *   IEEE  float   (format tag 3)  32-bit
 *   Multi-channel input is mixed down to mono.
 *
 * The signal is run through three pYIN contexts with different
 * voiced_transition_weight values so the effect of the parameter is
 * visible side-by-side.
 *
 * Compile:
 *   gcc -O2 -Wall -Wextra -o pyin_demo pyin_demo.c pyin.c -lm
 */

#include "pyinlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* =========================================================================
 * Minimal WAV loader
 * ========================================================================= */

typedef struct {
    float *samples;    /* heap-allocated, mono, normalised to [-1, +1] */
    int    n_samples;
    float  sample_rate;
} AudioBuffer;

static void audio_free(AudioBuffer *a)
{
    free(a->samples);
    a->samples   = NULL;
    a->n_samples = 0;
}

static uint16_t read_u16le(const uint8_t *p) {
    return (uint16_t)(p[0] | ((unsigned)p[1] << 8));
}
static uint32_t read_u32le(const uint8_t *p) {
    return (uint32_t)(p[0] | ((uint32_t)p[1]<<8)
                            | ((uint32_t)p[2]<<16)
                            | ((uint32_t)p[3]<<24));
}
static float read_f32le(const uint8_t *p) {
    uint32_t u = read_u32le(p);
    float f; memcpy(&f, &u, sizeof f);
    return f;
}

static bool wav_load(const char *path, AudioBuffer *out)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) { fprintf(stderr, "Cannot open '%s'\n", path); return false; }

    uint8_t hdr[12];
    if (fread(hdr, 1, 12, fp) != 12)             goto bad_fmt;
    if (memcmp(hdr,     "RIFF", 4) != 0)          goto bad_fmt;
    if (memcmp(hdr + 8, "WAVE", 4) != 0)          goto bad_fmt;

    uint16_t audio_format = 0, n_channels = 0, bits_per_sample = 0;
    uint32_t sample_rate  = 0, data_size  = 0;
    long     data_offset  = 0;

    while (1) {
        uint8_t chunk_hdr[8];
        if (fread(chunk_hdr, 1, 8, fp) != 8) break;
        char     id[5] = {0}; memcpy(id, chunk_hdr, 4);
        uint32_t size  = read_u32le(chunk_hdr + 4);
        long     start = ftell(fp);

        if (memcmp(id, "fmt ", 4) == 0) {
            if (size < 16) goto bad_fmt;
            uint8_t fmt[16];
            if (fread(fmt, 1, 16, fp) != 16) goto bad_fmt;
            audio_format    = read_u16le(fmt + 0);
            n_channels      = read_u16le(fmt + 2);
            sample_rate     = read_u32le(fmt + 4);
            bits_per_sample = read_u16le(fmt + 14);
        } else if (memcmp(id, "data", 4) == 0) {
            data_size   = size;
            data_offset = start;
            break;
        }
        fseek(fp, start + (long)size + ((long)size & 1), SEEK_SET);
    }

    if (data_offset == 0 || sample_rate == 0 || n_channels == 0) goto bad_fmt;
    if (audio_format != 1 && audio_format != 3) {
        fprintf(stderr, "Unsupported WAV format tag %u "
                        "(only PCM=1 and IEEE float=3 supported)\n",
                audio_format);
        fclose(fp); return false;
    }
    if (audio_format == 1 && bits_per_sample != 16
                           && bits_per_sample != 24
                           && bits_per_sample != 32) {
        fprintf(stderr, "Unsupported PCM bit depth %u\n", bits_per_sample);
        fclose(fp); return false;
    }
    if (audio_format == 3 && bits_per_sample != 32) {
        fprintf(stderr, "Only 32-bit IEEE float WAV supported\n");
        fclose(fp); return false;
    }

    uint32_t bps    = bits_per_sample / 8;
    uint32_t bpf    = bps * n_channels;
    int      nf     = (int)(data_size / bpf);

    fseek(fp, data_offset, SEEK_SET);
    uint8_t *raw = (uint8_t *)malloc(data_size);
    if (!raw) { fprintf(stderr, "Out of memory\n"); fclose(fp); return false; }
    size_t got = fread(raw, 1, data_size, fp);
    fclose(fp);
    if ((int)(got / bpf) < nf) nf = (int)(got / bpf);

    float *mono = (float *)malloc((size_t)nf * sizeof(float));
    if (!mono) { free(raw); return false; }

    for (int i = 0; i < nf; i++) {
        double sum = 0.0;
        for (int ch = 0; ch < n_channels; ch++) {
            const uint8_t *p = raw + (size_t)(i * n_channels + ch) * bps;
            double s = 0.0;
            if (audio_format == 3) {
                s = (double)read_f32le(p);
            } else if (bits_per_sample == 16) {
                int16_t v; memcpy(&v, p, 2);
                s = (double)v / 32768.0;
            } else if (bits_per_sample == 24) {
                int32_t v = (int32_t)(p[0] | ((uint32_t)p[1]<<8)
                                           | ((uint32_t)p[2]<<16));
                if (v & 0x800000) v |= (int32_t)0xFF000000;
                s = (double)v / 8388608.0;
            } else {
                int32_t v; memcpy(&v, p, 4);
                s = (double)v / 2147483648.0;
            }
            sum += s;
        }
        mono[i] = (float)(sum / n_channels);
    }

    free(raw);
    out->samples     = mono;
    out->n_samples   = nf;
    out->sample_rate = (float)sample_rate;

    printf("Loaded '%s': %.0f Hz, %d ch, %d-bit, %d samples (%.2f s)\n",
           path, (double)sample_rate, n_channels, bits_per_sample,
           nf, (double)nf / (double)sample_rate);
    return true;

bad_fmt:
    fprintf(stderr, "Not a valid WAV file: '%s'\n", path);
    fclose(fp);
    return false;
}

/* =========================================================================
 * Speech-like synthesiser
 *
 * Voiced source: bandlimited sawtooth wave (sum of harmonics) shaped by
 * a spectral envelope modelled as a cascade of biquad resonators (formants).
 * Unvoiced source: white noise passed through a shaping filter.
 *
 * The sawtooth produces sustained energy on every sample unlike a decaying
 * pulse train, so it survives the pYIN energy gate and gives the CMNDF a
 * clean periodic structure to latch onto.
 *
 * Segments:
 *   0.00–0.08 s  plosive burst    (broadband noise)
 *   0.08–0.55 s  vowel /a/        (F0 120→160 Hz, formants 800/1200/2500 Hz)
 *   0.55–0.70 s  nasal /m/        (F0 ~140 Hz,   formants  250/2500 Hz)
 *   0.70–0.90 s  fricative /s/    (high-frequency noise, unvoiced)
 *   0.90–0.95 s  stop gap         (silence)
 *   0.95–1.40 s  vowel /i/        (F0 160→130 Hz, formants 300/2200/3000 Hz)
 *   1.40–1.60 s  voiced /n/       (F0 ~130 Hz,   formants  250/2000 Hz)
 *   1.60–1.75 s  fricative /f/    (low-level broadband noise, unvoiced)
 *   1.75–2.00 s  silence
 * ========================================================================= */

/* Biquad state + coefficients packed together */
typedef struct {
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
} Biquad;

static void bq_bandpass(Biquad *bq, float freq, float bw, float sr)
{
    /* 2-pole resonator; peak gain = 1 at freq, -3 dB bandwidth = bw Hz */
    float R    = 1.0f - (float)M_PI * bw / sr;
    float cosw = cosf(2.0f * (float)M_PI * freq / sr);
    bq->b0 =  (1.0f - R*R) * 0.5f;
    bq->b1 =  0.0f;
    bq->b2 = -(1.0f - R*R) * 0.5f;
    bq->a1 = -2.0f * R * cosw;
    bq->a2 =  R * R;
    bq->x1 = bq->x2 = bq->y1 = bq->y2 = 0.0f;
}

static void bq_lowpass(Biquad *bq, float cutoff, float sr)
{
    /* Butterworth 2-pole low-pass */
    float w0   = 2.0f * (float)M_PI * cutoff / sr;
    float cosw = cosf(w0), sinw = sinf(w0);
    float alpha = sinw / (float)M_SQRT2;
    float b0 = (1.0f - cosw) * 0.5f;
    float b1 =  1.0f - cosw;
    float b2 = (1.0f - cosw) * 0.5f;
    float a0 =  1.0f + alpha;
    float a1 = -2.0f * cosw;
    float a2 =  1.0f - alpha;
    bq->b0 = b0/a0; bq->b1 = b1/a0; bq->b2 = b2/a0;
    bq->a1 = a1/a0; bq->a2 = a2/a0;
    bq->x1 = bq->x2 = bq->y1 = bq->y2 = 0.0f;
}

static void bq_highpass(Biquad *bq, float cutoff, float sr)
{
    float w0   = 2.0f * (float)M_PI * cutoff / sr;
    float cosw = cosf(w0), sinw = sinf(w0);
    float alpha = sinw / (float)M_SQRT2;
    float b0 =  (1.0f + cosw) * 0.5f;
    float b1 = -(1.0f + cosw);
    float b2 =  (1.0f + cosw) * 0.5f;
    float a0 =  1.0f + alpha;
    float a1 = -2.0f * cosw;
    float a2 =  1.0f - alpha;
    bq->b0 = b0/a0; bq->b1 = b1/a0; bq->b2 = b2/a0;
    bq->a1 = a1/a0; bq->a2 = a2/a0;
    bq->x1 = bq->x2 = bq->y1 = bq->y2 = 0.0f;
}

static inline float bq_tick(Biquad *bq, float x)
{
    float y = bq->b0*x + bq->b1*bq->x1 + bq->b2*bq->x2
                       - bq->a1*bq->y1  - bq->a2*bq->y2;
    bq->x2 = bq->x1; bq->x1 = x;
    bq->y2 = bq->y1; bq->y1 = y;
    return y;
}

/*
 * Bandlimited sawtooth via additive synthesis.
 * Sums harmonics k=1,2,...,K where K = floor(sr / (2*f0)).
 * Normalised to peak amplitude ≈ 1.
 */
static inline float blsaw(float phase, float f0, float sr)
{
    int   K   = (int)(sr / (2.0f * f0));
    if (K < 1) K = 1;
    float sum = 0.0f;
    for (int k = 1; k <= K; k++)
        sum += sinf(2.0f * (float)M_PI * (float)k * phase) / (float)k;
    /* Sawtooth has RMS = 1/sqrt(3); normalise to 0-dB peak ≈ ln(K+1)*2/pi */
    return sum * (float)(2.0 / M_PI / log(K + 2.0));
}

/* Park–Miller LCG — avoids stdlib rand() */
static uint32_t lcg = 0xACE1U;
static inline float randf(void)
{
    lcg = lcg * 1664525u + 1013904223u;
    return (float)(int32_t)lcg * (1.0f / 2147483648.0f);
}

/* Segment IDs – used to detect transitions and init filters exactly once */
enum Seg { SEG_NONE=-1, SEG_BURST=0, SEG_A, SEG_M, SEG_S,
           SEG_STOP, SEG_I, SEG_N, SEG_F, SEG_SIL };

static enum Seg seg_at(float t)
{
    if (t < 0.08f) return SEG_BURST;
    if (t < 0.55f) return SEG_A;
    if (t < 0.70f) return SEG_M;
    if (t < 0.90f) return SEG_S;
    if (t < 0.95f) return SEG_STOP;
    if (t < 1.40f) return SEG_I;
    if (t < 1.60f) return SEG_N;
    if (t < 1.75f) return SEG_F;
    return SEG_SIL;
}

static void synth_speech(float *buf, int n_total, float sr)
{
    Biquad f1, f2, f3, ns;
    memset(&f1, 0, sizeof f1);
    memset(&f2, 0, sizeof f2);
    memset(&f3, 0, sizeof f3);
    memset(&ns, 0, sizeof ns);

    float      phase   = 0.0f;
    enum Seg   prev    = SEG_NONE;

    for (int i = 0; i < n_total; i++) {
        float    t   = (float)i / sr;
        float    out = 0.0f;
        enum Seg seg = seg_at(t);

        /* ── One-time filter init on segment entry ─────────────────── */
        if (seg != prev) {
            switch (seg) {
            case SEG_A:
                bq_bandpass(&f1,  800.0f,  80.0f, sr);
                bq_bandpass(&f2, 1200.0f, 120.0f, sr);
                bq_bandpass(&f3, 2500.0f, 200.0f, sr);
                phase = 0.0f;
                break;
            case SEG_M:
                bq_bandpass(&f1, 250.0f,  50.0f, sr);
                bq_lowpass (&f2, 600.0f, sr);
                break;
            case SEG_S:
                bq_highpass(&ns, 4000.0f, sr);
                break;
            case SEG_I:
                bq_bandpass(&f1,  300.0f,  60.0f, sr);
                bq_bandpass(&f2, 2200.0f, 180.0f, sr);
                bq_bandpass(&f3, 3000.0f, 250.0f, sr);
                phase = 0.0f;
                break;
            case SEG_N:
                bq_bandpass(&f1, 250.0f,  45.0f, sr);
                bq_lowpass (&f2, 500.0f, sr);
                break;
            case SEG_F:
                bq_lowpass(&ns, 3000.0f, sr);
                break;
            default:
                break;
            }
            prev = seg;
        }

        switch (seg) {
        case SEG_BURST: {
            float env = (t < 0.025f) ? 1.0f : expf(-40.0f*(t-0.025f));
            out = randf() * 0.35f * env;
            break;
        }
        case SEG_A: {
            float alpha = (t - 0.08f) / 0.47f;
            float f0    = 120.0f + alpha * 40.0f;
            float env   = (alpha < 0.03f) ? alpha / 0.03f : 1.0f;
            phase += f0 / sr;
            if (phase >= 1.0f) phase -= 1.0f;
            float src = blsaw(phase, f0, sr) * env;
            out  = bq_tick(&f1, src) * 0.60f;
            out += bq_tick(&f2, src) * 0.40f;
            out += bq_tick(&f3, src) * 0.20f;
            out *= 0.55f;
            break;
        }
        case SEG_M: {
            float f0 = 140.0f;
            phase += f0 / sr;
            if (phase >= 1.0f) phase -= 1.0f;
            float src = blsaw(phase, f0, sr);
            out = bq_tick(&f2, bq_tick(&f1, src) * 0.5f) * 0.30f;
            break;
        }
        case SEG_S:
            out = bq_tick(&ns, randf()) * 0.25f;
            break;
        case SEG_STOP:
            out = 0.0f;
            break;
        case SEG_I: {
            float alpha = (t - 0.95f) / 0.45f;
            float f0    = 160.0f - alpha * 30.0f;
            float env   = (alpha < 0.02f) ? alpha / 0.02f : 1.0f;
            phase += f0 / sr;
            if (phase >= 1.0f) phase -= 1.0f;
            float src = blsaw(phase, f0, sr) * env;
            out  = bq_tick(&f1, src) * 0.55f;
            out += bq_tick(&f2, src) * 0.35f;
            out += bq_tick(&f3, src) * 0.15f;
            out *= 0.50f;
            break;
        }
        case SEG_N: {
            float f0 = 130.0f;
            phase += f0 / sr;
            if (phase >= 1.0f) phase -= 1.0f;
            float src = blsaw(phase, f0, sr);
            out = bq_tick(&f2, bq_tick(&f1, src) * 0.5f) * 0.28f;
            break;
        }
        case SEG_F:
            out = bq_tick(&ns, randf()) * 0.12f;
            break;
        case SEG_SIL:
        default:
            out = 0.0f;
            break;
        }

        buf[i] = out;
    }
}

/* =========================================================================
 * Pitch tracker runner
 * ========================================================================= */

static void run_audio(const float *samples, int n_samples, float sr,
                      float weight)
{
    PYINConfig cfg              = pyin_config_default();
    cfg.sample_rate             = sr;
    cfg.voiced_transition_weight = weight;
    cfg.cents_per_semitone = 10;
    cfg.f0_min = 80;
    cfg.octave_cost_weight = 2.;
    /* Scale frame/hop proportionally if sample rate differs from 44100 */
    // cfg.frame_size = (int)(sr * 2048.0f / 44100.0f + 0.5f);
    // cfg.frame_size = ((cfg.frame_size + cfg.block_size - 1)
    //                   / cfg.block_size) * cfg.block_size;
    cfg.frame_size = 2048;
    cfg.hop_size   = cfg.frame_size / 8;
    cfg.beta_b = 2.0f;
    cfg.energy_gate_rms = 1e-8f;

    PYINContext *ctx = pyin_create(cfg, NULL, NULL);
    if (!ctx) { fprintf(stderr, "pyin_create failed\n"); return; }

    printf("\n=== voiced_transition_weight = %.3f "
           "(frame=%d, hop=%d) ===\n",
           weight, cfg.frame_size, cfg.hop_size);
    printf("%-6s  %-7s  %-10s  %-7s  %s\n",
           "Frame", "t(s)", "pitch(Hz)", "conf", "state");
    printf("--------------------------------------------------\n");

    const int bsz = cfg.block_size;
    int frame = 0;

    for (int pos = 0; pos < n_samples; pos += bsz) {
        float block[64] = {0};
        int copy = n_samples - pos;
        if (copy > bsz) copy = bsz;
        memcpy(block, samples + pos, (size_t)copy * sizeof(float));

        PYINResult res;
        if (pyin_process_block(ctx, block, &res)) {
            printf("%-6d  %-7.3f  %-10.2f  %-7.4f  %s\n",
                   frame++,
                   (double)pos / (double)sr,
                   (double)res.pitch_hz,
                   (double)res.confidence,
                   res.voiced ? "voiced" : "unvoiced");
        }
    }

    pyin_destroy(ctx);
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(int argc, char *argv[])
{
    AudioBuffer audio = {NULL, 0, 0.0f};

    if (argc >= 2) {
        if (!wav_load(argv[1], &audio)) return 1;
    } else {
        float sr   = 44100.0f;
        int   n    = (int)(2.0f * sr);
        float *buf = (float *)malloc((size_t)n * sizeof(float));
        if (!buf) { fprintf(stderr, "Out of memory\n"); return 1; }
        synth_speech(buf, n, sr);
        audio.samples     = buf;
        audio.n_samples   = n;
        audio.sample_rate = sr;

        printf("Synthesised speech-like signal  (2.0 s @ 44100 Hz)\n");
        printf("  0.00-0.08 s  plosive burst\n");
        printf("  0.08-0.55 s  vowel /a/        voiced  F0 120→160 Hz\n");
        printf("  0.55-0.70 s  nasal /m/         voiced  F0 ~140 Hz\n");
        printf("  0.70-0.90 s  fricative /s/    unvoiced\n");
        printf("  0.90-0.95 s  stop closure     silence\n");
        printf("  0.95-1.40 s  vowel /i/         voiced  F0 160→130 Hz\n");
        printf("  1.40-1.60 s  nasal /n/         voiced  F0 ~130 Hz\n");
        printf("  1.60-1.75 s  fricative /f/    unvoiced\n");
        printf("  1.75-2.00 s  silence\n");
    }

    float weights[] = { 0.01f, 0.10f, 0.40f };
    for (int i = 0; i < 3; i++)
        run_audio(audio.samples, audio.n_samples, audio.sample_rate,
                  weights[i]);

    audio_free(&audio);
    return 0;
}
