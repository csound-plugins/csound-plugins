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

/* ── Ring buffer ────────────────────────────────────────────────────────── */

typedef struct {
    float   *buf;
    uint32_t cap;
    uint32_t write;
    uint32_t fill;
} RingBuffer;


// static bool ring_alloc(RingBuffer *r, int cap)
// {
//     r->buf   = (float *)calloc((size_t)cap, sizeof(float));
//     r->cap   = (uint32_t)cap;
//     r->write = 0;
//     r->fill  = 0;
//     return r->buf != NULL;
// }

static void ring_free(RingBuffer *r)  { free(r->buf); r->buf = NULL; }

static void ring_clear(RingBuffer *r)
{
    memset(r->buf, 0, r->cap * sizeof(float));
    r->write = 0;
    r->fill  = 0;
}

// static inline void ring_push(RingBuffer *r, const float *src, int n)
// {
//     uint32_t mask = r->cap - 1;
//     for (int i = 0; i < n; i++) {
//         r->buf[r->write & mask] = src[i];
//         r->write++;
//         if (r->fill < r->cap) r->fill++;
//     }
// }

// static inline void ring_read_latest(const RingBuffer *r, float *dst, int len)
// {
//     assert((uint32_t)len <= r->fill);
//     uint32_t mask  = r->cap - 1;
//     uint32_t start = (r->write - (uint32_t)len) & mask;
//     for (int i = 0; i < len; i++)
//         dst[i] = r->buf[(start + (uint32_t)i) & mask];
// }

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

void *_alloc(allocfn_t allocfn, void *ctx, size_t n, size_t size) {
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
 * Trellis arrays are flat [VITERBI_DEPTH × n_total].
 * back[] stores int16 predecessor indices; n_total ≤ 1761, fits in int16.
 */
typedef struct {
    float   *score;          /* [VITERBI_DEPTH * n_total]       */
    int16_t *back;           /* [VITERBI_DEPTH * n_total]       */
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
#define HMM_BACK(h, slot, s)   (h)->back [(slot) * (h)->n_total + (s)]

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

    h->score = (float *)_alloc(alloc_fn, alloc_ctx, VITERBI_DEPTH * h->n_total, sizeof(float));
    h->back = (int16_t *)_alloc(alloc_fn, alloc_ctx, VITERBI_DEPTH * h->n_total, sizeof(int16_t));
    h->log_trans_band = (float*)_alloc(alloc_fn, alloc_ctx, band_width, sizeof(float));

    // h->score = (float   *)calloc((size_t)(VITERBI_DEPTH * h->n_total),
    //                               sizeof(float));
    // h->back  = (int16_t *)calloc((size_t)(VITERBI_DEPTH * h->n_total),
    //                               sizeof(int16_t));
    // h->log_trans_band = (float *)malloc((size_t)band_width * sizeof(float));

    if (!h->score || !h->back || !h->log_trans_band) return false;

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
    // double *tmp = (double *)malloc((size_t)band_width * sizeof(double));
    // if (!tmp) return false;

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

    // free(tmp);

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

static void hmm_free(HMM *h)
{
    free(h->score);
    free(h->back);
    free(h->log_trans_band);
    h->score          = NULL;
    h->back           = NULL;
    h->log_trans_band = NULL;
}

static void hmm_clear(HMM *h)
{
    if (h->score) memset(h->score, 0,
                         (size_t)(VITERBI_DEPTH * h->n_total) * sizeof(float));
    if (h->back)  memset(h->back,  0,
                         (size_t)(VITERBI_DEPTH * h->n_total) * sizeof(int16_t));
    h->head   = 0;
    h->filled = 0;
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
            HMM_BACK (h, cur, s) = (int16_t)s;
        }
    } else {
        /* ── Voiced destination states ────────────────────────────────── */
        for (int s = 0; s < np; s++) {
            float best      = -1e30f;
            int   best_from = 0;

            /* From voiced states (banded Gaussian) */
            int f_lo = s - bh; if (f_lo < 0)   f_lo = 0;
            int f_hi = s + bh; if (f_hi >= np)  f_hi = np - 1;

            for (int f = f_lo; f <= f_hi; f++) {
                float v = HMM_SCORE(h, prev, f)
                        + h->log_trans_band[(f - s) + bh];
                if (v > best) { best = v; best_from = f; }
            }

            /* From unvoiced state */
            {
                float v = HMM_SCORE(h, prev, uv) + h->log_p_uv;
                if (v > best) { best = v; best_from = uv; }
            }

            HMM_SCORE(h, cur, s) = best + log_obs[s];
            HMM_BACK (h, cur, s) = (int16_t)best_from;
        }

        /* ── Unvoiced destination state ───────────────────────────────── */
        {
            float best      = -1e30f;
            int   best_from = 0;

            /* From any voiced state (flat cost p_vu, not banded) */
            for (int f = 0; f < np; f++) {
                float v = HMM_SCORE(h, prev, f) + h->log_p_vu;
                if (v > best) { best = v; best_from = f; }
            }

            /* From unvoiced state */
            {
                float v = HMM_SCORE(h, prev, uv) + h->log_p_uu;
                if (v > best) { best = v; best_from = uv; }
            }

            HMM_SCORE(h, cur, uv) = best + log_obs[uv];
            HMM_BACK (h, cur, uv) = (int16_t)best_from;
        }
    }

    h->head   = (cur + 1) % VITERBI_DEPTH;
    h->filled = (h->filled < VITERBI_DEPTH) ? h->filled + 1 : VITERBI_DEPTH;
}

/* Return the best state in the most recent trellis frame. */
static int hmm_best_state(const HMM *h)
{
    int   latest = (h->head - 1 + VITERBI_DEPTH) % VITERBI_DEPTH;
    float best   = -1e30f;
    int   best_s = 0;
    for (int s = 0; s < h->n_total; s++) {
        if (HMM_SCORE(h, latest, s) > best) {
            best   = HMM_SCORE(h, latest, s);
            best_s = s;
        }
    }
    return best_s;
}

/* ── State ↔ frequency ──────────────────────────────────────────────────── */

static inline float state_to_hz(int s, float state_cents)
{
    float cents = (float)(HMM_MIDI_MIN * 100) + (float)s * state_cents;
    return 440.0f * powf(2.0f, (cents - 6900.0f) / 1200.0f);
}

/* ── Beta distribution ──────────────────────────────────────────────────── */

static double beta_cdf(double x, double a, double b)
{
    if (x <= 0.0) return 0.0;
    if (x >= 1.0) return 1.0;

    double lbeta = lgamma(a) + lgamma(b) - lgamma(a + b);
    int swapped = 0;
    double aa = a, bb = b, xx = x;
    if (x > (a + 1.0) / (a + b + 2.0)) {
        xx = 1.0 - x; aa = b; bb = a; swapped = 1;
    }
    double front = exp(log(xx)*aa + log(1.0-xx)*bb - lbeta) / aa;
    double f = 1.0, C = 1.0, D = 0.0;
    for (int m = 0; m <= 200; m++) {
        for (int n = 0; n <= 1; n++) {
            double dm = (double)m, num;
            if (n == 0)
                num = (m==0) ? 1.0 : dm*(bb-dm)*xx /
                      ((aa+2.0*dm-1.0)*(aa+2.0*dm));
            else
                num = -(aa+dm)*(aa+bb+dm)*xx /
                       ((aa+2.0*dm)*(aa+2.0*dm+1.0));
            D = 1.0+num*D; if (fabs(D)<1e-30) D=1e-30;
            C = 1.0+num/C; if (fabs(C)<1e-30) C=1e-30;
            D = 1.0/D;
            double delta = C*D; f *= delta;
            if (fabs(delta-1.0)<1e-10) goto done;
        }
    }
done:;
    double r = front*(f-1.0);
    return swapped ? 1.0-r : r;
}

static inline float beta_exceed(float t, double a, double b)
{
    return (float)(1.0 - beta_cdf((double)t, a, b));
}

/* ── YIN ────────────────────────────────────────────────────────────────── */

static void compute_diff(const float *frame, int W,
                         float *diff, int max_lag)
{
    /*
     * Exact YIN difference function, computed efficiently with running sums.
     *
     *   d(tau) = sum_{j=0}^{W-tau-1} (x[j] - x[j+tau])^2
     *          = r_x(tau) + r_y(tau) - 2*r(tau)
     *
     * where:
     *   r_x(tau) = sum_{j=0}^{W-tau-1}   x[j]^2       (left sub-window energy)
     *   r_y(tau) = sum_{j=tau}^{W-1}      x[j]^2       (right sub-window energy)
     *   r(tau)   = sum_{j=0}^{W-tau-1}   x[j]*x[j+tau] (lagged cross-product)
     *
     * r_x and r_y can be maintained with O(1) updates per lag using the
     * following recurrences:
     *   r_x(0)   = sum_{j=0}^{W-1} x[j]^2
     *   r_x(tau) = r_x(tau-1) - x[W-tau]^2        (drop the last element)
     *
     *   r_y(0)   = sum_{j=0}^{W-1} x[j]^2         (same as r_x(0))
     *   r_y(tau) = r_y(tau-1) - x[tau-1]^2        (drop the first element)
     *
     * The lagged cross-product r(tau) must still be computed in O(W-tau)
     * per lag, but the inner loop is a simple dot product with no branches.
     *
     * Total cost: O(W) for the prefix sums + O(W * max_lag) for the
     * cross-products. For speech (max_lag ~ W/3), this is ~3× faster than
     * the naive form because the sub-window energy updates are free.
     *
     * Note: using per-lag energy (not a single global r(0)) gives much more
     * accurate CMNDF dips at large lags (low pitches) where the sub-windows
     * are significantly shorter than the full frame.
     */

    /* Compute full-frame energy = r_x(0) = r_y(0) */
    double r_x = 0.0, r_y = 0.0;
    for (int j = 0; j < W; j++) {
        double xj = (double)frame[j];
        r_x += xj * xj;
    }
    r_y = r_x;

    diff[0] = 0.0f;

    for (int tau = 1; tau <= max_lag; tau++) {
        /* Update sub-window energies: each loses one sample at the boundary */
        double drop_left  = (double)frame[W - tau];   /* r_x loses this */
        double drop_right = (double)frame[tau - 1];   /* r_y loses this */
        r_x -= drop_left  * drop_left;
        r_y -= drop_right * drop_right;

        /* Lagged cross-product: O(W - tau) */
        double rt = 0.0;
        int    n  = W - tau;
        for (int j = 0; j < n; j++)
            rt += (double)frame[j] * (double)frame[j + tau];

        diff[tau] = (float)(r_x + r_y - 2.0 * rt);
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
};

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
    c.octave_cost_weight        = 0.0f;
    c.octave_subharmonic_threshold = 3.0f;
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
    if (c->octave_cost_weight < 0.0f)                               return false;
    if (c->octave_subharmonic_threshold <= 1.0f)                    return false;
    return true;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

PYINContext *pyin_create(PYINConfig cfg, allocfn_t allocfn, freefn_t freefn, void *allocdata)
{
    if (!config_valid(&cfg)) return NULL;

    PYINContext *ctx = (PYINContext *)_alloc(allocfn, allocdata, 1, sizeof(PYINContext));
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

    ctx->ring.buf = _alloc(allocfn, allocdata, ring_cap, sizeof(float));
    if(!ctx->ring.buf) goto fail;
    ctx->ring.cap = ring_cap;
    ctx->ring.write = 0;
    ctx->ring.fill = 0;

    // if (!ring_alloc(&ctx->ring, ring_cap)) goto fail;

    size_t memsize = (size_t)cfg.frame_size + (size_t)lag_buf_len * 3 + (size_t)n_total;
    float *mem = (float *)_alloc(allocfn, allocdata, memsize, sizeof(float));
    if(!mem)
        goto fail;
    ctx->mem = mem;
    ctx->frame = mem;
    ctx->diff = mem + (size_t)cfg.frame_size;
    ctx->cmndf = ctx->diff + (size_t)lag_buf_len;
    ctx->p_voiced_lag = ctx->cmndf + (size_t)lag_buf_len;
    ctx->log_obs = ctx->p_voiced_lag + (size_t)lag_buf_len;

    // ctx->frame        = (float *)calloc((size_t)cfg.frame_size, sizeof(float));
    // ctx->diff         = (float *)calloc((size_t)lag_buf_len,    sizeof(float));
    // ctx->cmndf        = (float *)calloc((size_t)lag_buf_len,    sizeof(float));
    // ctx->p_voiced_lag = (float *)calloc((size_t)lag_buf_len,    sizeof(float));
    // ctx->log_obs      = (float *)malloc ((size_t)n_total        * sizeof(float));
    //
    if (!hmm_alloc(&ctx->hmm, ctx->n_pitched, ctx->band_half,
                   ctx->state_cents, cfg.voiced_transition_weight,
                   sigma_cents, ctx->allocfn, ctx->allocdata)) goto fail;

    return ctx;

fail:
    pyin_destroy(ctx);
    return NULL;
}

void pyin_destroy(PYINContext *ctx)
{
    if (!ctx) return;
    // ring_free(&ctx->ring);
    _free(ctx->freefn, ctx->allocdata, ctx->ring.buf);
    _free(ctx->freefn, ctx->allocdata, ctx->mem);
    // free(ctx->frame);
    // free(ctx->diff);
    // free(ctx->cmndf);
    // free(ctx->p_voiced_lag);
    // free(ctx->log_obs);
    // hmm_free(&ctx->hmm);

    _free(ctx->freefn, ctx->allocdata, ctx->hmm.score);
    _free(ctx->freefn, ctx->allocdata, ctx->hmm.back);
    _free(ctx->freefn, ctx->allocdata, ctx->hmm.log_trans_band);

    _free(ctx->freefn, ctx->allocdata, ctx);
    // free(ctx);
}

const PYINConfig *pyin_get_config(const PYINContext *ctx)
{
    return &ctx->cfg;
}

// void pyin_reset(PYINContext *ctx)
// {
//     ring_clear(&ctx->ring);
//     hmm_clear(&ctx->hmm);
//     ctx->samples_since_last_hop = 0;
// }

/* ── Core analysis ──────────────────────────────────────────────────────── */

static bool analyse_frame(PYINContext *ctx, PYINResult *result)
{
    const int   W           = ctx->cfg.frame_size;
    const int   lag_min     = ctx->lag_min;
    const int   lag_max     = ctx->lag_max;
    const int   n_pitched   = ctx->n_pitched;
    const float sample_rate = ctx->cfg.sample_rate;
    const float state_cents = ctx->state_cents;

    /* ── Energy gate ──────────────────────────────────────────────────────
     * Pure silence → d(tau)=0 → d'(tau)=0 → beta_exceed(0)=1 for all lags.
     * Short-circuit before any analysis to avoid feeding garbage to the HMM.
     * We still push an unvoiced-only observation so the trellis advances.
     * ──────────────────────────────────────────────────────────────────── */
    {
        double sum_sq = 0.0;
        for (int i = 0; i < W; i++)
            sum_sq += (double)ctx->frame[i] * (double)ctx->frame[i];

        if (sqrtf((float)(sum_sq / W)) < ctx->cfg.energy_gate_rms) {
            /* Feed a strongly unvoiced observation to the HMM */
            const float FLOOR = 1e-7f;
            for (int s = 0; s < n_pitched; s++)
                ctx->log_obs[s] = logf(FLOOR);
            ctx->log_obs[n_pitched] = 0.0f;   /* log(1) = 0 → certain unvoiced */
            hmm_push(&ctx->hmm, ctx->log_obs);

            result->pitch_hz   = 0.0f;
            result->confidence = 0.0f;
            result->voiced     = false;
            return true;
        }
    }

    /* ── Step 1: YIN difference + CMNDF ─────────────────────────────────── */
    compute_diff (ctx->frame, W, ctx->diff,  lag_max);
    compute_cmndf(ctx->diff,     ctx->cmndf, lag_max);

    /* ── Step 2: per-lag voiced probability ──────────────────────────────
     * p_voiced(τ) = P(Beta(a,b) > d'(τ)) = 1 – I_{d'(τ)}(a, b)
     * ──────────────────────────────────────────────────────────────────── */
    float max_p_voiced = 0.0f;
    for (int tau = lag_min; tau <= lag_max; tau++) {
        float v = ctx->cmndf[tau];
        if (v > 1.0f) v = 1.0f;
        float pv = beta_exceed(v, ctx->beta_a, ctx->beta_b);
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
    if (ocw > 0.0f) {
        const float K = ctx->cfg.octave_subharmonic_threshold;
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
        /* Recompute max after penalty */
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
     * ──────────────────────────────────────────────────────────────────── */
    const float FLOOR = 1e-7f;

    /* Apply voiced observation floor to the unvoiced observation only;
     * per-lag p_voiced values keep their original range for pitch accuracy. */
    float max_pv_floored = max_p_voiced;
    if (max_pv_floored < ctx->cfg.voiced_obs_floor)
        max_pv_floored = ctx->cfg.voiced_obs_floor;

    for (int s = 0; s < n_pitched; s++) {
        float hz    = state_to_hz(s, state_cents);
        float tau_f = sample_rate / hz;
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

    float hz_raw   = state_to_hz(best, state_cents);
    float tau_best = sample_rate / hz_raw;
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
