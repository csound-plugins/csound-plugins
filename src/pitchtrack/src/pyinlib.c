/*
 * pyin.c – Probabilistic YIN pitch tracker
 *
 * Build:
 *   gcc -O2 -march=native -Wall -Wextra -o pyin_demo pyin.c pyin_demo.c -lm
 *
 * HMM state layout
 * ─────────────────
 * States  0 … n_pitched-1  are voiced pitch states (fine cent grid).
 * State   n_pitched         is the single unvoiced state.
 * Total trellis width = n_pitched + 1  (stored as hmm.n_total).
 *
 * Voiced/unvoiced transition costs are derived from cfg.voiced_transition_weight
 * and stored as log-probabilities in the HMM at init time.  The Viterbi
 * smoother therefore controls boundary smoothness, not a post-hoc threshold.
 *
 * Transition matrix structure (log-prob):
 *
 *   from \ to  | voiced s'          | unvoiced
 *   -----------+--------------------+------------------
 *   voiced s   | log_trans_band[Δs] | log_p_vu
 *   unvoiced   | log_p_uv           | log_p_uu
 *
 * where:
 *   p_vu  = voiced_transition_weight          (voiced   → unvoiced)
 *   p_uv  = voiced_transition_weight          (unvoiced → voiced, spread over all pitch states)
 *   p_vv  = 1 - p_vu                          (voiced   → stay voiced, split by Gaussian)
 *   p_uu  = 1 - n_pitched * (p_uv / n_pitched) = 1 - p_uv_total
 */

#include "pyinlib.h"

#include "kiss_fft.h"
#include "kiss_fftr.h"
#ifdef FIXED_POINT
#error "pyinlib requires floating-point kiss_fft_scalar"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
// #include <assert.h>
#include <stdint.h>
#include <stdio.h>

/* ── Internal fixed constants ───────────────────────────────────────────── */

/* Transition band cutoff: ±N×sigma — at ±4σ the Gaussian is ~0.0003 of peak */
#define TRANS_BAND_SIGMA    4

#define VITERBI_DEPTH       20

#define HMM_MIDI_MIN        21
#define HMM_SEMITONES       88

/* ── Helpers ────────────────────────────────────────────────────────────── */

static int next_pow2(int x)
{
    int p = 1;
    while (p < x) p <<= 1;
    return p;
}

typedef struct {
    double a, b;
    double lbeta;
    double threshold;
} beta_cdf_ctx;

/* ── Ring buffer ────────────────────────────────────────────────────────── */

typedef struct {
    float   *buf;
    uint32_t cap;
    uint32_t write;
    uint32_t fill;
} RingBuffer;


static inline void ring_push(RingBuffer *r, const float *src, int n)
{
    uint32_t mask  = r->cap - 1;
    uint32_t write = r->write & mask;
    uint32_t cap   = r->cap;

    // Update fill once, outside the loop
    uint32_t new_fill = r->fill + (uint32_t)n;
    r->fill = new_fill < cap ? new_fill : cap;

    // Check if the write wraps around the ring boundary
    uint32_t space_to_end = cap - write;
    if ((uint32_t)n <= space_to_end) {
        // No wrap: single memcpy
        memcpy(r->buf + write, src, (size_t)n * sizeof(float));
    } else {
        memcpy(r->buf + write, src,                (size_t)space_to_end * sizeof(float));
        memcpy(r->buf,         src + space_to_end, (size_t)(n - space_to_end) * sizeof(float));
    }

    r->write += (uint32_t)n;
}

static inline void ring_read_latest(const RingBuffer *r, float *dst, int len)
{
    // assert((uint32_t)len <= r->fill);
    uint32_t cap   = r->cap;
    uint32_t mask  = cap - 1;
    uint32_t start = (r->write - (uint32_t)len) & mask;

    uint32_t chunk1 = cap - start;   // floats available before wrap
    if ((uint32_t)len <= chunk1) {
        // No wrap: single memcpy
        memcpy(dst, r->buf + start, (size_t)len * sizeof(float));
    } else {
        memcpy(dst,          r->buf + start, (size_t)chunk1 * sizeof(float));
        memcpy(dst + chunk1, r->buf,         (size_t)(len - chunk1) * sizeof(float));
    }
}

void *_calloc(allocfn_t allocfn, void *ctx, size_t n, size_t size) {
    if(ctx && allocfn) {
        return allocfn(ctx, n*size);
    } else {
        return calloc(n, size);
    }
}

void _free(freefn_t freefn, void *ctx, void *ptr) {
    if(ctx && freefn)
        freefn(ctx, ptr);
    else
        free(ptr);
}


/* ── HMM ────────────────────────────────────────────────────────────────── */

/*
 * n_total = n_pitched + 1.
 * The unvoiced state is always the last index: UNVOICED_IDX = n_pitched.
 *
 * Trellis scores are flat [VITERBI_DEPTH × n_total].  No backpointer array
 * is kept: decoding reads only the best state of the latest frame
 * (hmm_best_state), so predecessor indices would be write-only.
 */
typedef struct {
    float   *score;          /* [VITERBI_DEPTH * n_total]       */
    int      head;
    int      filled;

    int      n_pitched;      /* number of voiced pitch states   */
    int      n_total;        /* n_pitched + 1  (includes unvoiced) */
    int      band_half;

    /* Voiced↔voiced transition band (shift-invariant Gaussian) */
    float   *log_trans_band; /* [2*band_half + 1]               */

    /* Voiced↔unvoiced / unvoiced↔voiced scalar log-probs */
    float    log_p_vu;   /* log p(voiced s → unvoiced)          */
    float    log_p_vv;   /* log p(voiced s → any voiced s')
                            (to be added to the Gaussian weight) */
    float    log_p_uv;   /* log p(unvoiced → specific voiced s) */
    float    log_p_uu;   /* log p(unvoiced → unvoiced)          */
} HMM;

#define HMM_SCORE(h, slot, s)  (h)->score[(slot) * (h)->n_total + (s)]

#define MAX_BANDWIDTH 8192

static bool hmm_alloc(HMM *h, int n_pitched, int band_half,
                      float state_cents, float voiced_transition_weight,
                      double sigma_cents, allocfn_t alloc_fn, void *alloc_ctx)
{
    h->n_pitched = n_pitched;
    h->n_total   = n_pitched + 1;      /* +1 for the unvoiced state */
    h->band_half = band_half;
    h->head      = 0;
    h->filled    = 0;

    int band_width = 2 * band_half + 1;
    if(band_width > MAX_BANDWIDTH)
        return false;

    h->log_trans_band = (float*)_calloc(alloc_fn, alloc_ctx, band_width, sizeof(float));
    h->score = (float *)_calloc(alloc_fn, alloc_ctx, VITERBI_DEPTH * h->n_total, sizeof(float));
    if (!h->score || !h->log_trans_band) return false;

    /* ── Voiced→voiced Gaussian band ────────────────────────────────────
     *
     * Normalisation convention:
     *   log_trans_band[d=0] = log(p_vv)              (self-transition)
     *   log_trans_band[d]   = log(p_vv) + log_gauss(d) - log_gauss(0)
     *                       = log(p_vv) - 0.5*(d*state_cents/sigma)^2
     *
     * This means staying on the same pitch costs only log(1 - p_vu) ≈ 0
     * per frame, while moving by k semitones costs an additional
     * 0.5*(k*100/sigma)^2 nats.  The total voiced→voiced mass (summed
     * over all voiced targets) slightly exceeds p_vv because the Gaussian
     * is truncated at ±4σ, but the error is negligible (<0.1%).
     *
     * Why not normalise so the band sums exactly to p_vv?
     * That would make the self-transition weight p_vv/band_width ≈ 0.012,
     * which is 80× weaker than the unvoiced self-loop (p_uu ≈ 0.99), making
     * voiced segments permanently penalised and preventing onset detection.
     */
    double tmp[MAX_BANDWIDTH];

    double p_vu = (double)voiced_transition_weight;
    double p_vv = 1.0 - p_vu;

    for (int d = -band_half; d <= band_half; d++) {
        double cents_dist = d * (double)state_cents;
        double sigma      = sigma_cents;
        /* log p(to=s+d | from=s, voiced→voiced)
         *   = log(p_vv) - 0.5*(cents_dist/sigma)^2                        */
        tmp[d + band_half] = log(p_vv)
                           - 0.5 * (cents_dist / sigma) * (cents_dist / sigma);
    }

    for (int i = 0; i < band_width; i++)
        h->log_trans_band[i] = (float)tmp[i];

    /* ── Voiced ↔ unvoiced transition log-probs ──────────────────────────
     *
     * p(voiced s → unvoiced)          = p_vu
     * p(unvoiced  → any voiced s)     = p_uv   (total)
     * p(unvoiced  → specific voiced s): NOT normalised by n_pitched here.
     *   The observation log_obs[s] differentiates between voiced targets;
     *   the Viterbi max picks the best one.  Using log(p_uv/n_pitched) would
     *   add an extra -log(n_pitched) ≈ -6.8 nats per frame and permanently
     *   suppress the unvoiced→voiced transition.
     * p(unvoiced  → unvoiced)         = 1 - p_uv
     */
    double p_uv = p_vu;   /* symmetric */
    double p_uu = 1.0 - p_uv;

    h->log_p_vu = (float)log(p_vu);
    h->log_p_vv = (float)log(p_vv);   /* kept for reference; not used directly */
    h->log_p_uv = (float)log(p_uv);   /* NOT divided by n_pitched */
    h->log_p_uu = (float)log(p_uu);

    return true;
}

/*
 * Advance the Viterbi trellis by one frame.
 *
 * log_obs[0 … n_pitched-1] : observation log-likelihoods for voiced states
 * log_obs[n_pitched]        : observation log-likelihood for unvoiced state
 *
 * Transition structure:
 *   voiced s  → voiced s'  : log_trans_band[s'-s + band_half]  (banded Gaussian)
 *   voiced s  → unvoiced   : log_p_vu
 *   unvoiced  → voiced s'  : log_p_uv  (same for every s')
 *   unvoiced  → unvoiced   : log_p_uu
 */
static void hmm_push(HMM *h, const float *log_obs)
{
    int np  = h->n_pitched;
    int nu  = h->n_total;        /* = np + 1              */
    int uv  = np;                /* index of unvoiced state */
    int bh  = h->band_half;
    int cur  = h->head;
    int prev = (cur - 1 + VITERBI_DEPTH) % VITERBI_DEPTH;

    if (h->filled == 0) {
        /* Uniform prior over all states (voiced + unvoiced) */
        float log_prior = -logf((float)nu);
        for (int s = 0; s < nu; s++) {
            HMM_SCORE(h, cur, s) = log_prior + log_obs[s];
        }
    } else {
        /* NOTE: no backpointers are stored. Decoding (hmm_best_state) reads
         * only the latest trellis row, so predecessor indices would be
         * write-only traffic. */
        /* ── Voiced destination states ────────────────────────────────── */
        const float * restrict log_trans_band = h->log_trans_band;
        for (int s = 0; s < np; s++) {
            float best = -1e30f;

            /* From voiced states (banded Gaussian) */
            int f_lo = s - bh; if (f_lo < 0)   f_lo = 0;
            int f_hi = s + bh; if (f_hi >= np)  f_hi = np - 1;

            for (int f = f_lo; f <= f_hi; f++) {
                float v = HMM_SCORE(h, prev, f)
                        + log_trans_band[(f - s) + bh];
                if (v > best) best = v;
            }

            /* From unvoiced state */
            {
                float v = HMM_SCORE(h, prev, uv) + h->log_p_uv;
                if (v > best) best = v;
            }

            HMM_SCORE(h, cur, s) = best + log_obs[s];
        }

        /* ── Unvoiced destination state ───────────────────────────────── */
        {
            float best = -1e30f;

            /* From any voiced state (flat cost p_vu, not banded) */
            for (int f = 0; f < np; f++) {
                float v = HMM_SCORE(h, prev, f) + h->log_p_vu;
                if (v > best) best = v;
            }

            /* From unvoiced state */
            {
                float v = HMM_SCORE(h, prev, uv) + h->log_p_uu;
                if (v > best) best = v;
            }

            HMM_SCORE(h, cur, uv) = best + log_obs[uv];
        }
    }

    h->head   = (cur + 1) % VITERBI_DEPTH;
    h->filled = (h->filled < VITERBI_DEPTH) ? h->filled + 1 : VITERBI_DEPTH;
}

/* Return the best state in the most recent trellis frame. */
static inline int hmm_best_state(const HMM *h)
{
    int   latest = (h->head - 1 + VITERBI_DEPTH) % VITERBI_DEPTH;
    float best   = -1e30f;
    int   best_s = 0;
    for (int s = 0; s < h->n_total; s++) {
        float hmmscore = HMM_SCORE(h, latest, s);
        int cond = (hmmscore > best);
        best = cond ? hmmscore : best;
        best_s = cond ? s : best_s;
    }
    return best_s;
}

/* ── State ↔ frequency ──────────────────────────────────────────────────── */

static inline float state_to_hz(int s, float state_cents)
{
    float cents = (float)(HMM_MIDI_MIN * 100) + (float)s * state_cents;
    return 440.0f * powf(2.0f, (cents - 6900.0f) / 1200.0f);
}

/* ── Beta distribution ────────────────────────────────────────────────────
 *
 * beta_cdf_eval (continued fraction) runs once per LUT entry at init time.
 * The per-lag hot path uses the precomputed beta_lut instead.
 */

static inline double beta_cdf_eval(const beta_cdf_ctx *ctx, double x)
{
    if (x <= 0.0) return 0.0;
    if (x >= 1.0) return 1.0;

    int swap = x > ctx->threshold;

    double xx = swap ? (1.0 - x) : x;
    double aa = swap ? ctx->b : ctx->a;
    double bb = swap ? ctx->a : ctx->b;

    double logx  = log(xx);
    double log1x = log1p(-xx);

    double front = exp(logx * aa + log1x * bb - ctx->lbeta) / aa;

    double f = 1.0, C = 1.0, D = 0.0;

    for (int m = 0; m <= 200; m++) {
        double dm = (double)m;
        double a2m = aa + 2.0 * dm;

        // even step
        double num;
        if (m == 0) {
            num = 1.0;
        } else {
            num = dm * (bb - dm) * xx / ((a2m - 1.0) * a2m);
        }

        D = 1.0 + num * D;
        C = 1.0 + num / C;

        D = copysign(fmax(fabs(D), 1e-30), D);
        C = copysign(fmax(fabs(C), 1e-30), C);

        D = 1.0 / D;
        double delta = C * D;
        f *= delta;

        if (fabs(delta - 1.0) < 1e-10) break;

        // odd step
        num = -(aa + dm) * (aa + bb + dm) * xx / (a2m * (a2m + 1.0));

        D = 1.0 + num * D;
        C = 1.0 + num / C;

        D = copysign(fmax(fabs(D), 1e-30), D);
        C = copysign(fmax(fabs(C), 1e-30), C);

        D = 1.0 / D;
        delta = C * D;
        f *= delta;

        if (fabs(delta - 1.0) < 1e-10) break;
    }

    double r = front * (f - 1.0);
    return swap ? (1.0 - r) : r;
}

/* ── YIN ────────────────────────────────────────────────────────────────── */

#include <stddef.h>

/* Portable force-inline */
#if defined(_MSC_VER)
#  define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#  define FORCE_INLINE __attribute__((always_inline)) inline
#else
#  define FORCE_INLINE inline
#endif

/* Horizontal sum of 4 accumulators — the compiler folds this into
   a single SIMD reduction when the loop above was vectorized.      */
static FORCE_INLINE float hsum4(float a0, float a1, float a2, float a3)
{
    return (a0 + a1) + (a2 + a3);
}

/* Dot product of two float arrays, length n.
 *
 * Four independent accumulators break the serial add-dependency so
 * the compiler can issue 4 FMAs per cycle.  The tail (n % 4 != 0)
 * is handled scalarly.  With -O2 / -O3 the loop body will be
 * auto-vectorized to SSE/AVX on x86 or NEON on ARM.
 */
static FORCE_INLINE float dot_product(const float * restrict a,
                                      const float * restrict b,
                                       int n)
{
    /* NOTE: buffers are not guaranteed 32-byte aligned (the ring buffer,
     * sub-slices and lag offsets all break alignment), so no alignment
     * assumptions here; the loop still auto-vectorizes with unaligned loads. */
    float acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;
    int j = 0;

    for (; j <= n - 4; j += 4) {
        acc0 += a[j + 0] * b[j + 0];
        acc1 += a[j + 1] * b[j + 1];
        acc2 += a[j + 2] * b[j + 2];
        acc3 += a[j + 3] * b[j + 3];
    }
    /* Scalar tail */
    float acc = hsum4(acc0, acc1, acc2, acc3);
    for (; j < n; j++)
        acc += a[j] * b[j];

    return acc;
}

/* Full-frame energy, vectorized via the 4-accumulator dot product. */
static inline float frame_energy(const float * restrict frame, int W)
{
    return dot_product(frame, frame, W);
}

/*
 * compute_diff_direct — YIN difference function, direct O(W*maxlag) form.
 *
 *   d(tau) = r_x(tau) + r_y(tau) - 2*r(tau),
 * with r_x/r_y maintained by O(1) recurrences and r(tau) a dot product.
 * r_x0 is the full-frame energy sum(x[j]^2), computed once by the caller
 * (frame_energy) and shared with the energy gate, so the frame is only
 * summed once.  r_y(0) equals r_x(0).
 *
 * No alignment requirement: the inner dot products use unaligned loads.
 *
 * See compute_diff_fft for the FFT-based equivalent (same contract).
 */
static void compute_diff_direct(const float * restrict frame, int W,
                                float       * restrict diff,  int max_lag,
                                float r_x0)
{
    const float *af = frame;
    /* --- full-frame energy r_x(0) = r_y(0), provided by the caller --- */
    float r_x = r_x0, r_y = r_x0;
    diff[0] = 0.0f;

    for (int tau = 1; tau <= max_lag; tau++) {
        /* O(1) energy updates — intentionally scalar, no loop to vectorize */
        float drop_left  = af[W - tau];
        float drop_right = af[tau - 1];
        r_x -= drop_left  * drop_left;
        r_y -= drop_right * drop_right;

        /* Lagged cross-product — the hot path.
         * dot_product() is inlined; the 4-accumulator unroll lets the
         * compiler emit 4-wide FMA instructions (vfmadd* / fmla).     */
        float rt = dot_product(af, af + tau, W - tau);
        diff[tau] = r_x + r_y - 2.0f * rt;
    }

}

/*
 * compute_diff_fft — YIN difference function via FFT autocorrelation.
 *
 * Same contract as compute_diff_direct: identical d(tau) up to float
 * rounding.  r(tau) for all lags comes from one zero-padded autocorrelation:
 * forward real FFT, magnitude-square the spectrum, inverse FFT, scale by
 * 1/N.  Padding to N >= W + maxlag keeps bins 1..maxlag free of circular
 * wrap-around, so r(tau) matches the direct sum exactly in exact arithmetic.
 * r_x/r_y use the same O(1) recurrences as the direct path.  Tiny negative
 * results from FFT rounding are clamped to 0 (the direct path can also
 * produce ~1e-9 negatives; both feed the same downstream clamps).
 */
static void compute_diff_fft(int N, float * restrict work,
                             kiss_fft_cpx * restrict spec,
                             kiss_fftr_cfg fwd, kiss_fftr_cfg inv,
                             const float * restrict frame, int W,
                             float       * restrict diff,  int max_lag,
                             float r_x0)
{
    memcpy(work, frame, (size_t)W * sizeof(float));
    memset(work + W, 0, (size_t)(N - W) * sizeof(float));

    kiss_fftr(fwd, work, spec);
    for (int k = 0; k <= N / 2; k++) {
        float re = spec[k].r, im = spec[k].i;
        spec[k].r = re * re + im * im;
        spec[k].i = 0.0f;
    }
    kiss_fftri(inv, spec, work);

    const float inv_n = 1.0f / (float)N;
    float r_x = r_x0, r_y = r_x0;
    diff[0] = 0.0f;

    for (int tau = 1; tau <= max_lag; tau++) {
        float drop_left  = frame[W - tau];
        float drop_right = frame[tau - 1];
        r_x -= drop_left  * drop_left;
        r_y -= drop_right * drop_right;

        float rt = work[tau] * inv_n;
        float d  = r_x + r_y - 2.0f * rt;
        diff[tau] = d > 0.0f ? d : 0.0f;
    }
}


static void compute_cmndf(const float *diff, float *cmndf, int max_lag)
{
    cmndf[0] = 1.0f;
    double running = 0.0;
    for (int tau = 1; tau <= max_lag; tau++) {
        running += (double)diff[tau];
        cmndf[tau] = (running > 0.0)
                   ? (float)((double)diff[tau] * tau / running)
                   : 0.0f;
    }
}

static float parabolic_interp(const float *cmndf, int tau, int max_lag)
{
    if (tau <= 0 || tau >= max_lag) return (float)tau;
    float s0 = cmndf[tau-1], s1 = cmndf[tau], s2 = cmndf[tau+1];
    float denom = s0 - 2.0f*s1 + s2;
    if (fabsf(denom) < 1e-9f) return (float)tau;
    return (float)tau + 0.5f * (s0 - s2) / denom;
}

/* Linear interpolation of the CMNDF at a (possibly fractional) lag x.
 * Used by the subharmonic penalty to evaluate cmndf(τ/k). */
static float cmndf_interp(const float *cmndf, float x, int max_lag)
{
    if (x <= 0.0f)              return cmndf[0];
    if (x >= (float)max_lag)    return cmndf[max_lag];
    int   i    = (int)x;
    float frac = x - (float)i;
    return (1.0f - frac) * cmndf[i] + frac * cmndf[i + 1];
}

/* ── Main context ───────────────────────────────────────────────────────── */

struct PYINContext {
    PYINConfig cfg;

    /* Derived */
    int    lag_min;
    int    lag_max;
    int    n_pitched;    /* voiced pitch states  = HMM_SEMITONES * cps       */
    int    band_half;
    float  state_cents;
    double beta_a;       /* cached from cfg for hot-path use                 */
    double beta_b;

    /* Runtime */
    int samples_since_last_hop;
    int prev_voiced;      /* previous analysis frame decoded as voiced */
    int hold_left;        /* voiced_obs_hold hangover frames remaining */
    int hold_len;         /* hangover length in frames (set at create)     */

    /* Heap buffers */
    RingBuffer ring;
    float *mem;
    float *frame;
    float *diff;
    float *cmndf;
    float *p_voiced_lag;
    float *log_obs;      /* [n_pitched + 1]: voiced[0..n_pitched-1], unvoiced[n_pitched] */

    HMM hmm;

    allocfn_t allocfn;
    freefn_t freefn;
    void *allocdata;
    beta_cdf_ctx betacdf;
    /* Exceedance LUT: beta_lut[i] = P(Beta(a,b) > i/BETA_LUT_N).
     * a/b are fixed per config, so the per-lag continued fraction is
     * replaced by a table lookup + lerp (measured ~100x faster,
     * max abs err ~1e-6 with N=2048). */
    float *beta_lut;
    /* Per-state period in samples: state_tau[s] = sample_rate / hz(s).
     * The pitch grid never changes, so Step 3 + decode avoid a powf and
     * a division per state per frame. */
    float *state_tau;
    /* FFT difference-function workspace (only when cfg.diff_use_fft).
     * fft_n is the padded length (next pow2 >= frame + maxlag);
     * fft_work holds N real samples, fft_spec N/2+1 complex bins. */
    int            fft_n;
    float         *fft_work;
    kiss_fft_cpx  *fft_spec;
    kiss_fftr_cfg  fft_fwd;
    kiss_fftr_cfg  fft_inv;
};

#define BETA_LUT_N 2048

static inline float beta_lut_exceed(const PYINContext *ctx, float v)
{
    float vc = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    float x  = vc * (float)BETA_LUT_N;
    int   i  = (int)x;
    if (i >= BETA_LUT_N) return ctx->beta_lut[BETA_LUT_N];
    float f = x - (float)i;
    float lo = ctx->beta_lut[i];
    return lo + f * (ctx->beta_lut[i + 1] - lo);
}

/* ── Default config ─────────────────────────────────────────────────────── */

PYINConfig pyin_config_default(void)
{
    PYINConfig c;
    c.sample_rate               = 44100.0f;
    c.block_size                = 64;
    c.frame_size                = 2048;
    c.hop_size                  = 512;
    c.f0_min                    = 60.0f;
    c.f0_max                    = 900.0f;
    c.cents_per_semitone        = 10;
    c.voiced_transition_weight  = 0.1f;
    c.pitch_sigma_cents         = 100.0f;
    c.beta_a                    = 2.0f;
    c.beta_b                    = 6.0f;
    c.energy_gate_rms           = 1e-4f;
    c.voiced_obs_floor          = 0.0f;
    c.voiced_obs_hold           = 0.0f;
    c.octave_cost_weight        = 0.0f;
    c.octave_subharmonic_threshold = 3.0f;
    c.subharmonic_cost_weight   = 0.0f;
    c.diff_use_fft              = true;
    return c;
}

/* ── Validation ─────────────────────────────────────────────────────────── */

static bool config_valid(const PYINConfig *c)
{
    if (c->block_size  <= 0)                                    return false;
    if (c->frame_size  <= 0)                                    return false;
    if (c->hop_size    <= 0)                                    return false;
    if (c->hop_size     > c->frame_size)                        return false;
    if (c->hop_size    % c->block_size != 0)                    return false;
    if (c->sample_rate <= 0.0f)                                 return false;
    if (c->f0_min      <= 0.0f)                                 return false;
    if (c->f0_min      >= c->f0_max)                            return false;
    if (c->f0_max      >= c->sample_rate / 2.0f)                return false;
    if (c->frame_size  < (int)(2.0f * c->sample_rate / c->f0_min))
                                                                return false;
    if (c->cents_per_semitone <= 0)                             return false;
    // if (100 % c->cents_per_semitone != 0)                       return false;
    if (c->voiced_transition_weight <= 0.0f)                    return false;
    if (c->voiced_transition_weight >  1.0f)                    return false;
    if (c->pitch_sigma_cents        <= 0.0f)                    return false;
    if (c->beta_a <= 0.0f)                                      return false;
    if (c->beta_b <= 0.0f)                                      return false;
    if (c->energy_gate_rms < 0.0f)                             return false;
    if (c->voiced_obs_floor < 0.0f || c->voiced_obs_floor >= 0.5f) return false;
    if (c->voiced_obs_hold  < 0.0f || c->voiced_obs_hold  >= 0.95f) return false;
    if (c->octave_cost_weight < 0.0f)                               return false;
    if (c->subharmonic_cost_weight < 0.0f)                         return false;
    if (c->octave_subharmonic_threshold <= 1.0f)                    return false;
    return true;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

PYINContext *pyin_create(PYINConfig cfg, allocfn_t allocfn, freefn_t freefn, void *allocdata)
{
    if (!config_valid(&cfg)) return NULL;

    PYINContext *ctx = (PYINContext *)_calloc(allocfn, allocdata, 1, sizeof(PYINContext));
    if (!ctx) return NULL;

    ctx->allocfn = allocfn;
    ctx->allocdata = allocdata;
    ctx->freefn = freefn;

    ctx->cfg         = cfg;
    ctx->lag_min     = (int)(cfg.sample_rate / cfg.f0_max + 0.5f);
    ctx->lag_max     = (int)(cfg.sample_rate / cfg.f0_min + 0.5f);
    ctx->n_pitched   = HMM_SEMITONES * cfg.cents_per_semitone;
    ctx->state_cents = 100.0f / (float)cfg.cents_per_semitone;
    ctx->beta_a      = (double)cfg.beta_a;
    ctx->beta_b      = (double)cfg.beta_b;

    /* Voiced-hold hangover: about one analysis window, so a glide whose
     * smearing lasts one window is bridged but longer silences are not. */
    ctx->hold_len    = cfg.frame_size / cfg.hop_size + 3;
    ctx->hold_left   = 0;

    /* Band half-width: ±TRANS_BAND_SIGMA × sigma, rounded up to whole states.
     * Capped so it never exceeds the full pitch range. */
    double sigma_cents = (double)cfg.pitch_sigma_cents;
    int band_half = (int)(TRANS_BAND_SIGMA * sigma_cents
                          / (double)ctx->state_cents + 0.5);
    if (band_half < 1) band_half = 1;
    if (band_half > ctx->n_pitched / 2) band_half = ctx->n_pitched / 2;
    ctx->band_half = band_half;

    int ring_cap    = next_pow2(cfg.frame_size + cfg.block_size);
    int lag_buf_len = ctx->lag_max + 2;
    int n_total     = ctx->n_pitched + 1;   /* +1 for unvoiced */

    ctx->ring.buf = _calloc(allocfn, allocdata, ring_cap, sizeof(float));
    if(!ctx->ring.buf) goto fail;
    ctx->ring.cap = ring_cap;
    ctx->ring.write = 0;
    ctx->ring.fill = 0;

    size_t memsize = (size_t)cfg.frame_size + (size_t)lag_buf_len * 3 + (size_t)n_total;
    /* one chunk, carved into frame/diff/cmndf/p_voiced_lag/log_obs */
    float *mem = (float *)_calloc(allocfn, allocdata, memsize, sizeof(float));
    if(!mem)
        goto fail;
    ctx->mem = mem;
    ctx->frame = mem;
    ctx->diff = mem + (size_t)cfg.frame_size;
    ctx->cmndf = ctx->diff + (size_t)lag_buf_len;
    ctx->p_voiced_lag = ctx->cmndf + (size_t)lag_buf_len;
    ctx->log_obs = ctx->p_voiced_lag + (size_t)lag_buf_len;

    if (!hmm_alloc(&ctx->hmm, ctx->n_pitched, ctx->band_half,
                   ctx->state_cents, cfg.voiced_transition_weight,
                   sigma_cents, ctx->allocfn, ctx->allocdata)) goto fail;


    double a = ctx->beta_a;
    double b = ctx->beta_b;
    ctx->betacdf.a = a;
    ctx->betacdf.b = b;
    ctx->betacdf.lbeta = lgamma(a) + lgamma(b) - lgamma(a + b);
    ctx->betacdf.threshold = (a + 1.0) / (a + b + 2.0);

    /* Build the exceedance LUT once; hot path only does lerp. */
    ctx->beta_lut = (float *)_calloc(allocfn, allocdata,
                                    (size_t)BETA_LUT_N + 1, sizeof(float));
    if (!ctx->beta_lut) goto fail;
    for (int i = 0; i <= BETA_LUT_N; i++)
        ctx->beta_lut[i] =
            (float)(1.0 - beta_cdf_eval(&ctx->betacdf,
                                        (double)i / (double)BETA_LUT_N));

    ctx->state_tau = (float *)_calloc(allocfn, allocdata,
                                     (size_t)ctx->n_pitched, sizeof(float));
    if (!ctx->state_tau) goto fail;
    for (int s = 0; s < ctx->n_pitched; s++)
        ctx->state_tau[s] = cfg.sample_rate / state_to_hz(s, ctx->state_cents);

    /* FFT workspace for the autocorrelation path (create-time only, so the
     * hot path never allocates).  N must cover frame + max lag. */
    ctx->fft_n = 0;
    ctx->fft_work = NULL;
    ctx->fft_spec = NULL;
    ctx->fft_fwd = NULL;
    ctx->fft_inv = NULL;
    if (cfg.diff_use_fft) {
        int fft_n = next_pow2(cfg.frame_size + ctx->lag_max);
        if (fft_n < 4) goto fail;
        ctx->fft_work = (float *)_calloc(allocfn, allocdata,
                                         (size_t)fft_n, sizeof(float));
        ctx->fft_spec = (kiss_fft_cpx *)_calloc(allocfn, allocdata,
                                               (size_t)(fft_n / 2 + 1),
                                               sizeof(kiss_fft_cpx));
        if (!ctx->fft_work || !ctx->fft_spec) goto fail;
        ctx->fft_fwd = kiss_fftr_alloc(fft_n, 0, NULL, NULL);
        ctx->fft_inv = kiss_fftr_alloc(fft_n, 1, NULL, NULL);
        if (!ctx->fft_fwd || !ctx->fft_inv) goto fail;
        ctx->fft_n = fft_n;
    }

    return ctx;

fail:
    pyin_destroy(ctx);
    return NULL;
}

void pyin_destroy(PYINContext *ctx)
{
    if (!ctx) return;
    _free(ctx->freefn, ctx->allocdata, ctx->ring.buf);
    _free(ctx->freefn, ctx->allocdata, ctx->mem);
    _free(ctx->freefn, ctx->allocdata, ctx->beta_lut);
    _free(ctx->freefn, ctx->allocdata, ctx->state_tau);
    _free(ctx->freefn, ctx->allocdata, ctx->fft_work);
    _free(ctx->freefn, ctx->allocdata, ctx->fft_spec);
    /* kiss_fft cfgs use malloc/free internally; only ever created/destroyed
     * here, never on the audio path. */
    free(ctx->fft_fwd);
    free(ctx->fft_inv);
    _free(ctx->freefn, ctx->allocdata, ctx->hmm.score);
    _free(ctx->freefn, ctx->allocdata, ctx->hmm.log_trans_band);

    _free(ctx->freefn, ctx->allocdata, ctx);
}

const PYINConfig *pyin_get_config(const PYINContext *ctx)
{
    return &ctx->cfg;
}

/* ── Core analysis ──────────────────────────────────────────────────────── */

static bool analyse_frame(PYINContext *ctx, PYINResult *result)
{
    const int   W           = ctx->cfg.frame_size;
    const int   lag_min     = ctx->lag_min;
    const int   lag_max     = ctx->lag_max;
    const int   n_pitched   = ctx->n_pitched;
    const float sample_rate = ctx->cfg.sample_rate;

    /* ── Energy gate ──────────────────────────────────────────────────────
     * Pure silence → d(tau)=0 → d'(tau)=0 → p_voiced=1 for all lags.
     * Short-circuit before any analysis to avoid feeding garbage to the HMM.
     * We still push an unvoiced-only observation so the trellis advances.
     *
     * The frame energy is computed once here (vectorized) and reused as
     * r_x(0) by compute_diff below, so the frame is summed only once.
     * Comparing energy against gate^2*W avoids the sqrt/divide; the gate
     * decision is identical up to float rounding of the sum.
     * ──────────────────────────────────────────────────────────────────── */
    float frame_e;
    {
        float gate = ctx->cfg.energy_gate_rms;
        frame_e = frame_energy(ctx->frame, W);

        if (frame_e < gate * gate * (float)W) {
            /* Feed a strongly unvoiced observation to the HMM */
            const float FLOOR = 1e-7f;
            const float logfloor = logf(FLOOR);
            for (int s = 0; s < n_pitched; s++)
                ctx->log_obs[s] = logfloor;
            ctx->log_obs[n_pitched] = 0.0f;   /* log(1) = 0 → certain unvoiced */
            hmm_push(&ctx->hmm, ctx->log_obs);

            ctx->prev_voiced   = 0;
            ctx->hold_left     = 0;
            result->pitch_hz   = 0.0f;
            result->confidence = 0.0f;
            result->voiced     = false;
            return true;
        }
    }

    /* ── Step 1: YIN difference + CMNDF ─────────────────────────────────── */
    if (ctx->cfg.diff_use_fft)
        compute_diff_fft(ctx->fft_n, ctx->fft_work, ctx->fft_spec,
                         ctx->fft_fwd, ctx->fft_inv,
                         ctx->frame, W, ctx->diff, lag_max, frame_e);
    else
        compute_diff_direct(ctx->frame, W, ctx->diff, lag_max, frame_e);
    compute_cmndf(ctx->diff,     ctx->cmndf, lag_max);

    /* ── Step 2: per-lag voiced probability ──────────────────────────────
     * p_voiced(τ) = P(Beta(a,b) > d'(τ)) = 1 – I_{d'(τ)}(a, b)
     * ──────────────────────────────────────────────────────────────────── */
    float max_p_voiced = 0.0f;
    for (int tau = lag_min; tau <= lag_max; tau++) {
        /* beta_lut_exceed clamps to [0,1] and lerps the precomputed table */
        float pv = beta_lut_exceed(ctx, ctx->cmndf[tau]);
        ctx->p_voiced_lag[tau] = pv;
        if (pv > max_p_voiced) max_p_voiced = pv;
    }

    /* ── Octave-consistency penalty ───────────────────────────────────────
     *
     * Octave errors occur when a signal has a weak or missing fundamental
     * (F0) but strong 2nd harmonic (2×F0).  The CMNDF then dips more
     * strongly at lag τ = T/2 (period of 2×F0) than at τ = T (period of F0),
     * causing the tracker to lock onto 2×F0 instead of F0.
     *
     * Detection: for each candidate lag τ, check whether τ×2 (the period of
     * the sub-harmonic, i.e. half the frequency) also has a competitive dip.
     * If cmndf[2τ] is within octave_subharmonic_threshold × cmndf[τ], the
     * shorter lag τ is likely a harmonic alias of the true lower pitch, so
     * p_voiced[τ] is penalised.
     *
     * Formula:
     *   R = cmndf[2τ] / cmndf[τ]
     *   if R < octave_subharmonic_threshold:
     *     p_voiced[τ] *= (R / octave_subharmonic_threshold)^octave_cost_weight
     *   else:
     *     no penalty (sub-harmonic is much weaker → τ is likely the true pitch)
     *
     * Applied only when 2τ <= lag_max (sub-harmonic must be in the search range).
     * ──────────────────────────────────────────────────────────────────── */
    const float ocw = ctx->cfg.octave_cost_weight;
    const float scw = ctx->cfg.subharmonic_cost_weight;
    if (ocw > 0.0f || scw > 0.0f) {
        const float K = ctx->cfg.octave_subharmonic_threshold;

        if (ocw > 0.0f) {
            for (int tau = lag_min; tau <= lag_max; tau++) {
                int tau2 = tau * 2;
                if (tau2 > lag_max) continue;   /* sub-harmonic out of range */
                float ct  = ctx->cmndf[tau];
                float c2t = ctx->cmndf[tau2];
                if (ct <= 0.0f) continue;
                float R = c2t / ct;
                if (R < K) {
                    /* sub-harmonic is competitive: τ may be a harmonic alias */
                    float penalty = powf(R / K, ocw);    /* in (0, 1) */
                    ctx->p_voiced_lag[tau] *= penalty;
                }
            }
        }

        /* ── Subharmonic (octave-down / first-dip) penalty ────────────────
         *
         * Mirror of the octave penalty above.  Here we detect a candidate τ
         * that is a subharmonic (lower octave) of a shorter lag τ/k that is a
         * *strictly deeper* CMNDF dip.  This is the common failure after a fast
         * jump to a higher pitch: the Viterbi cannot reach the new fundamental
         * in one transition (it lies outside the ±4σ band) and every
         * subharmonic of it has p_voiced ≈ 1, so the path settles on the
         * subharmonic nearest the old pitch.  Penalising all such lags lets the
         * tracker leave them.
         *
         *   ρ = cmndf(τ/k) / cmndf(τ)      (only ρ < 1 means "deeper")
         *   p_voiced[τ] *= ρ ^ subharmonic_cost_weight
         *
         * Requiring a strictly deeper divisor is what keeps this from harming
         * real voiced speech: there the fundamental and its 2nd harmonic have
         * comparable dips (ρ ≈ 1), so the penalty is negligible, whereas a
         * clean subharmonic lock has ρ ≪ 1.  The smallest ρ over k is used, and
         * a true fundamental — whose divisors are not dips at all — is left
         * untouched.  The penalty is continuous at ρ = 1, so there is no cliff.
         * ──────────────────────────────────────────────────────────────── */
        if (scw > 0.0f) {
            /* Lags whose voiced probability is already negligible can never be
             * selected, so penalising them cannot change the result; skipping
             * them keeps the divisor scan off the hot path. */
            const float CAND = 1e-4f;
            for (int tau = lag_min; tau <= lag_max; tau++) {
                float c_here = ctx->cmndf[tau];
                if (c_here <= 0.0f) continue;
                if (ctx->p_voiced_lag[tau] < CAND) continue;

                /* Track the smallest divisor ratio; the penalty is monotonic
                 * in ρ, so only a single powf is needed per lag. */
                float invc   = 1.0f / c_here;
                float minRho = 1.0f;
                for (int k = 2; ; k++) {
                    float x = (float)tau / (float)k;
                    if (x < (float)lag_min) break;
                    float cdiv = cmndf_interp(ctx->cmndf, x, lag_max);
                    float rho  = cdiv * invc;    /* = cmndf(τ/k) / cmndf(τ) */
                    if (rho < minRho) minRho = rho;
                }
                if (minRho < 1.0f)
                    ctx->p_voiced_lag[tau] *= powf(minRho, scw); /* in (0,1) */
            }
        }

        /* Recompute max once after all penalties */
        max_p_voiced = 0.0f;
        for (int tau = lag_min; tau <= lag_max; tau++) {
            if (ctx->p_voiced_lag[tau] > max_p_voiced)
                max_p_voiced = ctx->p_voiced_lag[tau];
        }
    }

    /* ── Step 3: HMM observation log-likelihoods ─────────────────────────
     *
     * Voiced states: log p(obs | voiced s) = log p_voiced(τ(s))
     *   interpolated between bracketing integer lags.
     *
     * Unvoiced state: log p(obs | unvoiced) = log(1 - max_p_voiced)
     *   The maximum per-lag voiced probability is the best evidence of any
     *   periodicity; its complement is the evidence for aperiodicity.
     *
     * voiced_obs_floor: clamp max_p_voiced from below so a single bad frame
     *   (creak, glottalization, microphone noise) cannot impose an arbitrarily
     *   large penalty on the voiced path.  Does not affect per-lag values used
     *   for the individual voiced-state observations.
     *
     * voiced_obs_hold: the same clamp, but only while the previous frame was
     *   decoded voiced.  This is voicing hysteresis: it bridges short
     *   low-periodicity stretches inside a voiced segment (e.g. a fast pitch
     *   glide) without making voiced onsets and offsets harder to detect.
     * ──────────────────────────────────────────────────────────────────── */
    const float FLOOR = 1e-7f;

    /* Apply voiced observation floor to the unvoiced observation only;
     * per-lag p_voiced values keep their original range for pitch accuracy. */
    /* Voiced-hold hangover: refresh while the frame still carries solid
     * periodicity, otherwise count down.  The hold then bridges a short
     * low-evidence stretch (a glide smeared across one window) but expires
     * during longer silences / genuine unvoiced segments. */
    if (max_p_voiced >= 0.4f)
        ctx->hold_left = ctx->hold_len;
    else if (ctx->hold_left > 0)
        ctx->hold_left--;

    float obs_floor = ctx->cfg.voiced_obs_floor;
    if (ctx->prev_voiced && ctx->hold_left > 0 && ctx->cfg.voiced_obs_hold > obs_floor) {
        /* Only hold while the frame is clearly above the energy gate: the
         * gate itself catches silence, but borderline frames just above it
         * can carry spurious periodicity that the hold would otherwise latch
         * onto.  Requiring the frame energy to exceed 4× the gate (i.e. RMS
         * above 2× gate) keeps the hold for real speech only. */
        float gate = ctx->cfg.energy_gate_rms;
        float gate_e = 4.0f * gate * gate * (float)W;
        if (frame_e > gate_e)
            obs_floor = ctx->cfg.voiced_obs_hold;
    }

    float max_pv_floored = max_p_voiced;
    if (max_pv_floored < obs_floor)
        max_pv_floored = obs_floor;

    for (int s = 0; s < n_pitched; s++) {
        float tau_f = ctx->state_tau[s];
        int   t0    = (int)tau_f;
        int   t1    = t0 + 1;
        float p;

        if (t0 >= lag_min && t1 <= lag_max) {
            float alpha = tau_f - (float)t0;
            p = (1.0f - alpha) * ctx->p_voiced_lag[t0]
              +          alpha  * ctx->p_voiced_lag[t1];
        } else if (t0 >= lag_min && t0 <= lag_max) {
            p = ctx->p_voiced_lag[t0];
        } else {
            p = FLOOR;
        }

        ctx->log_obs[s] = logf(p < FLOOR ? FLOOR : p);
    }

    /* Unvoiced observation: uses floored value to bound voiced-path debt */
    float p_unvoiced = 1.0f - max_pv_floored;
    ctx->log_obs[n_pitched] = logf(p_unvoiced < FLOOR ? FLOOR : p_unvoiced);

    /* ── Step 4: banded Viterbi update ──────────────────────────────────── */
    hmm_push(&ctx->hmm, ctx->log_obs);

    /* ── Step 5: decode ──────────────────────────────────────────────────── */
    int best = hmm_best_state(&ctx->hmm);
    ctx->prev_voiced = (best != n_pitched);

    /*
     * If the best state is the unvoiced state: report unvoiced.
     * If it is a voiced pitch state: extract frequency and confidence.
     */
    if (best == n_pitched) {
        result->pitch_hz   = 0.0f;
        result->confidence = 0.0f;
        result->voiced     = false;
        return true;
    }

    float tau_best = ctx->state_tau[best];
    float hz_raw   = sample_rate / tau_best;
    int   tau_i    = (int)roundf(tau_best);
    if (tau_i < lag_min) tau_i = lag_min;
    if (tau_i > lag_max) tau_i = lag_max;

    /* ── Step 6: confidence ──────────────────────────────────────────────── */
    /*
     * Confidence = p_voiced at the best lag.
     * The HMM has already made the voiced/unvoiced decision; this value
     * reflects the strength of the periodicity evidence at the decoded pitch.
     */
    float confidence = ctx->p_voiced_lag[tau_i];
    if (confidence > 1.0f) confidence = 1.0f;

    float refined_tau = parabolic_interp(ctx->cmndf, tau_i, lag_max);
    float pitch_hz    = (refined_tau > 0.5f)
                      ? sample_rate / refined_tau
                      : hz_raw;

    result->confidence = confidence;
    result->voiced     = true;
    result->pitch_hz   = pitch_hz;
    return true;
}

/* ── Public entry point ─────────────────────────────────────────────────── */

bool pyin_process_block(PYINContext   *ctx,
                        const float   *samples,
                        PYINResult    *result)
{
    ring_push(&ctx->ring, samples, ctx->cfg.block_size);
    ctx->samples_since_last_hop += ctx->cfg.block_size;

    if ((int)ctx->ring.fill < ctx->cfg.frame_size)
        return false;

    if (ctx->samples_since_last_hop < ctx->cfg.hop_size)
        return false;

    ctx->samples_since_last_hop = 0;
    ring_read_latest(&ctx->ring, ctx->frame, ctx->cfg.frame_size);
    return analyse_frame(ctx, result);
}
