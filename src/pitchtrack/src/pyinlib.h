
#ifndef PYIN_H
#define PYIN_H

#include <stdlib.h>
#include <stdbool.h>

/* =========================================================================
 * pYIN – Probabilistic YIN pitch estimator
 * Real-time C implementation
 *
 * Reference:
*   Mauch, M. & Dixon, S. (2014). pYIN: A Fundamental Frequency Estimator
 *   Using Probabilistic Threshold Distributions. ICASSP 2014.
 * ========================================================================= */


/* ── Configuration ──────────────────────────────────────────────────────── */

/**
 * All tuneable parameters in one struct.  Fill it with pyin_config_default(),
 * adjust any fields, then pass it to pyin_create().
 *
 * Constraints (checked in pyin_create; returns NULL on violation):
 *   block_size  > 0
 *   frame_size  > 0  and  frame_size >= 2 * (sample_rate / f0_min)
 *   hop_size    > 0  and  hop_size <= frame_size
 *   hop_size must be an exact multiple of block_size
 *   f0_min > 0  and  f0_min < f0_max
 *   f0_max < sample_rate / 2
 *   cents_per_semitone in {1, 2, 4, 5, 10, 20, 25, 50, 100}
 *   voiced_transition_weight in (0, 1]
 */
typedef struct {
    float sample_rate;          /* Hz  (e.g. 44100, 48000)                   */
    int   block_size;           /* samples per pyin_process_block() call      */
    int   frame_size;           /* analysis window length in samples          */
    int   hop_size;             /* samples between successive analysis frames */
    float f0_min;               /* Hz – lowest detectable pitch               */
    float f0_max;               /* Hz – highest detectable pitch              */

    /**
     * HMM pitch grid resolution (subdivisions per semitone).
     *
     * F0 candidates are quantised to a uniform grid in cents spanning the
     * configured [f0_min, f0_max] range.  Finer grids track vibrato and
     * glides more accurately and reduce Viterbi quantisation error.  The
     * banded Viterbi is O(states × 8σ) per frame so cost scales linearly
     * with the number of states, not quadratically.
     *
     *   1  → 100 cents/state  coarse, very fast
     *   2  →  50 cents/state
     *   4  →  25 cents/state
     *  10  → 10 cents/state  recommended default
     *  20  →   5 cents/state  fine
     *
     * The state count also depends on the f0 range; e.g. the default
     * 60–900 Hz range at 10 cents/state gives roughly 470 states.
     */
    int   cents_per_semitone;

    /**
     * Voiced ↔ unvoiced transition weight  ∈  (0, 1].
     *
     * The HMM contains an explicit unvoiced state alongside the pitch states.
     * This parameter controls the probability of crossing the voiced/unvoiced
     * boundary in a single frame, and therefore how smoothly the tracker
     * commits to or abandons voiced segments.
     *
     * Interpretation:
     *   voiced_transition_weight = p(voiced→unvoiced) = p(unvoiced→voiced)
     *
     * Lower values → higher cost to switch → longer, more stable voiced/unvoiced
     *                segments; fewer spurious detections in noisy speech but
     *                slower to react to real onsets and offsets.
     *
     * Higher values → lower cost to switch → fast reaction to voicing changes;
     *                 may produce more flickering on borderline frames.
     *
     * Recommended starting range: 0.01 (very smooth) … 0.3 (very reactive).
     * Default: 0.1
     */
    float voiced_transition_weight;

    /**
     * Pitch transition smoothness: standard deviation of the Gaussian used
     * for voiced→voiced state transitions, in cents.
     *
     * This is the single most direct control over octave jumps and
     * pitch-track continuity between frames.
     *
     * The Viterbi transition cost for moving from pitch p to pitch p±k cents
     * between adjacent frames is:
     *
     *   cost = 0.5 × (k / pitch_sigma_cents)²   nats
     *
     * Smaller σ → steeper penalty for pitch movement → smoother tracks,
     *             strongly suppresses octave jumps, but may lag behind
     *             fast legitimate glides.
     * Larger σ  → shallower penalty → allows faster pitch changes,
     *             more susceptible to octave errors on ambiguous frames.
     *
     * The band half-width is automatically set to ±4σ, so changing this
     * also adjusts how many states are considered in the Viterbi sweep.
     *
     * Practical ranges:
     *   50  cents (½ semitone) – very smooth; good for sustained notes,
     *                            monophonic instruments, or clean studio voice
     *   100 cents (1 semitone) – default; suitable for normal speech
     *   200 cents (2 semitones) – allows moderate glides and vibrato
     *   500 cents (5 semitones) – very loose; wide vibrato or large leaps
     *
     * For octave-jump suppression in speech: try 50–75 cents.
     * Default: 100.0 (1 semitone)
     */
    float pitch_sigma_cents;  /* Default: 100.0f */

    /**
     * Beta distribution shape parameters for the probabilistic threshold.
     *
     * Per-lag voicing probability is computed as:
     *   p_voiced(τ) = P(Beta(beta_a, beta_b) > CMNDF(τ))
     *               = 1 − I_{CMNDF(τ)}(beta_a, beta_b)
     *
     * These parameters control the break-even CMNDF value at which a lag
     * is considered equally likely to be voiced or unvoiced:
     *
     *   break-even ≈ (beta_a − 1) / (beta_a + beta_b − 2)   (mode of Beta)
     *
     * Lower beta_b (relative to beta_a) → more lenient → suited for real
     * speech where CMNDF minima are rarely below 0.05 due to noise and jitter.
     *
     * Higher beta_b → stricter → better for clean synthetic or studio signals.
     *
     * Defaults (2.0, 6.0) are calibrated for the exact YIN difference function
     * used here: d(τ) = Σ(x[j]−x[j+τ])², which produces higher CMNDF minima
     * than the approximation in the original Mauch & Dixon paper.
     *
     * If tracking clean synthetic signals, try (2.0, 18.0) for a tighter gate.
     */
    float beta_a;   /* Default: 2.0 */
    float beta_b;   /* Default: 6.0 */

    /**
     * Energy gate: frames with RMS below this level are forced to unvoiced
     * without running the CMNDF or updating the HMM with voiced evidence.
     *
     * The gate prevents silence (d(τ)=0 everywhere → CMNDF=0 → p_voiced=1)
     * from being classified as voiced.
     *
     * If you are seeing correct-pitch frames being marked unvoiced, especially
     * with quiet recordings, AGC-processed audio, or soft-spoken speech:
     * lower this value (e.g. 1e-5 or even 0 to disable).
     *
     * If silence frames are producing spurious voiced detections:
     * raise this value (e.g. 1e-3).
     *
     * Rule of thumb: set to ~10× the RMS noise floor of your quiet segments.
     * Default: 1e-4  (≈ −77 dBFS peak)
     */
    float energy_gate_rms;  /* Default: 1e-4f */

    /**
     * Voiced observation floor: lower bound clamped onto max_p_voiced before
     * deriving the unvoiced state's observation.
     *
     * The unvoiced observation is normally log(1 − max_p_voiced).  Without a
     * floor, a single genuinely ambiguous frame (creak, glottalization,
     * breath) where the CMNDF is high can produce max_p_voiced ≈ 0.15, i.e.
     * p_unvoiced ≈ 0.85, enough to let the unvoiced path overtake the voiced
     * path; with the default voiced_transition_weight=0.10 it then takes 8–10
     * strong frames to recover.
     *
     * Setting voiced_obs_floor=0.20 means: "treat the voiced probability as at
     * least 0.20", i.e. cap p_unvoiced at 0.80, for every frame.  This bounds
     * the debt a single bad frame can impose on the voiced path.
     *
     * Interpretation of values:
     *   0.00  – disabled (default)
     *   0.10  – mild smoothing
     *   0.20  – recommended for natural speech with occasional creaky voice,
     *           breathiness, or microphone noise
     *   0.30+ – increasingly aggressive; may prevent detection of genuinely
     *           unvoiced short segments embedded in voiced speech
     *
     * This is an unconditional bias.  For bridging short low-periodicity
     * frames *inside* a voiced segment (e.g. a fast pitch glide) without also
     * making voiced onsets harder to detect, use voiced_obs_hold instead.
     *
     * Default: 0.0 (disabled)
     */
    float voiced_obs_floor;  /* Default: 0.0f */

    /**
     * Voicing hysteresis: conditional observation floor while voiced.
     *
     * A fast pitch glide (or any brief, low-periodicity transition) can make
     * the CMNDF dip shallow for several consecutive frames, so max_p_voiced
     * drops and the HMM commits to unvoiced even though the signal is
     * clearly voiced.  Widening the analysis window does not help (the
     * opposite), and a global voiced_obs_floor large enough to bridge it
     * makes voiced onsets and offsets unreliable.
     *
     * This parameter adds hysteresis instead: while the previous analysis
     * frame was decoded as voiced, max_p_voiced is clamped up to at least
     * this value (p_unvoiced capped accordingly).  Entering a voiced region
     * is unaffected, but once voiced, a short run of weak frames no longer
     * forces an unvoiced dropout.  The hold is released as soon as a frame
     * decodes unvoiced (or hits the energy gate).
     *
     *   0.0  – disabled (default; previous behaviour)
     *   0.6  – mild hold
     *   0.8  – bridges the ≈30 ms glides seen in continuous speech
     *   0.9  – strong hold; may smear genuine short unvoiced intervals
     *
     * Values are clamped to [0, 0.95).
     *
     * Default: 0.0 (disabled)
     */
    float voiced_obs_hold;  /* Default: 0.0f */

    /**
     * Octave error suppression: penalty weight.
     *
     * Octave errors arise when a voice has a weak or missing fundamental (F0)
     * but strong energy at the 2nd harmonic (2×F0).  The CMNDF then dips more
     * strongly at the shorter lag (higher frequency), causing the tracker to
     * lock onto 2×F0 — an octave above the perceptual pitch.
     *
     * Detection: for each candidate lag τ, the double lag 2τ is checked.
     * If cmndf[2τ] is within `octave_subharmonic_threshold` × cmndf[τ],
     * the sub-harmonic (half frequency) is considered competitive, meaning
     * τ is likely a harmonic alias rather than the true pitch.  The per-lag
     * voiced probability is then scaled down by:
     *
     *   penalty = (cmndf[2τ] / (octave_subharmonic_threshold × cmndf[τ]))
     *             ^ octave_cost_weight
     *
     * Only applied when 2τ is within the lag search range (i.e. the
     * sub-harmonic pitch is above f0_min).
     *
     * Values:
     *   0.0  – disabled (default)
     *   1.0  – moderate; good starting point for speech
     *   2.0  – aggressive; nearly eliminates octave errors but may slightly
     *          reduce sensitivity for genuinely high pitches
     *
     * Default: 0.0 (disabled)
     */
    float octave_cost_weight;  /* Default: 0.0f */

    /**
     * Octave error suppression: sub-harmonic competitiveness threshold.
     *
     * Controls how close the sub-harmonic dip (cmndf[2τ]) must be to the
     * candidate dip (cmndf[τ]) for the penalty to activate.
     *
     * If cmndf[2τ] / cmndf[τ] < octave_subharmonic_threshold → penalise τ.
     * If cmndf[2τ] / cmndf[τ] ≥ octave_subharmonic_threshold → no penalty.
     *
     * Intuition: a ratio of 2.0 means "if the sub-harmonic dip is less than
     * twice as bad as the candidate dip, call it competitive."  Higher values
     * are more aggressive (penalise even when the sub-harmonic is quite weak).
     *
     * Recommended range: 2.0 – 5.0
     * Default: 3.0
     */
    float octave_subharmonic_threshold;  /* Default: 3.0f */

    /**
     * Subharmonic (octave-down) lock suppression: penalty weight.
     *
     * The mirror of octave_cost_weight, and a separate knob because it
     * addresses the opposite failure mode:
     *
     *   octave_cost_weight     – candidate reports 2×F0 (too high); suppressed
     *                            by checking the double lag 2τ.
     *   subharmonic_cost_weight – candidate reports F0/k (too low), typically
     *                            after a fast jump to a higher pitch; the
     *                            Viterbi cannot reach the new fundamental
     *                            because the nearest reachable lag is a
     *                            subharmonic of it, and the spectral emission
     *                            of every subharmonic saturates at p_voiced≈1.
     *
     * Detection (first-dip preference, generalised to any integer subharmonic):
     * for each candidate lag τ and each divisor k = 2, 3, … with τ/k ≥ lag_min,
     * compare the divisor dip cmndf(τ/k) with the candidate dip cmndf(τ).  Only
     * a *strictly deeper* shorter dip (ρ < 1) counts, so a true fundamental —
     * whose divisors are not dips at all, or are only as deep as it is — is
     * left untouched:
     *
     *   ρ = cmndf(τ/k) / cmndf(τ)
     *   p_voiced[τ] *= ρ ^ subharmonic_cost_weight        (only when ρ < 1)
     *
     * The smallest ρ over all divisors is used.  Requiring ρ < 1 is what keeps
     * this safe on real voiced speech: there the fundamental and its 2nd
     * harmonic have comparable dips (ρ ≈ 1), so the penalty is negligible,
     * whereas a clean subharmonic lock has ρ ≪ 1.  The penalty is continuous at
     * ρ = 1, so there is no threshold cliff.
     *
     * Values:
     *   0.0  – disabled (default; preserves previous behaviour)
     *   0.5  – mild; safe for continuous voiced speech
     *   1.0  – resolves subharmonic locks after low→high jumps
     *   2.0  – aggressive; stronger suppression, may bias very low pitches
     *
     * Default: 0.0
     */
    float subharmonic_cost_weight;  /* Default: 0.0f */

    /**
     * Difference-function backend.
     *
     *   true  (default) – lagged cross-correlation via FFT (zero-padded
     *           autocorrelation): O(N log N), roughly 2-3x faster per frame.
     *   false – direct O(W*maxlag) dot products; kept for comparison,
     *           debugging, and minimal-memory builds (no FFT workspace).
     *
     * Both paths compute the same mathematical quantity; the test suite
     * pins them to identical musical decisions.  Read once at pyin_create;
     * changing it afterwards has no effect.
     */
    bool diff_use_fft;  /* Default: true */

} PYINConfig;

/**
 * Return a PYINConfig filled with sensible defaults:
 *   sample_rate               = 44100 Hz
 *   block_size                = 64
 *   frame_size                = 2048
 *   hop_size                  = 512
 *   f0_min                    = 60 Hz
 *   f0_max                    = 900 Hz
 *   cents_per_semitone        = 10
 *   voiced_transition_weight  = 0.1
 *   pitch_sigma_cents         = 100.0
 *   beta_a                    = 2.0
 *   beta_b                    = 6.0
 *   energy_gate_rms           = 1e-4
 *   voiced_obs_floor          = 0.0
 *   voiced_obs_hold           = 0.0
 *   octave_cost_weight        = 0.0
 *   octave_subharmonic_threshold = 3.0
 *   subharmonic_cost_weight   = 0.0
 *   diff_use_fft              = true
 */
PYINConfig pyin_config_default(void);

/* ── Result ─────────────────────────────────────────────────────────────── */

typedef struct {
    float pitch_hz;   /* estimated F0; 0 when unvoiced                       */
    float confidence; /* posterior probability of voicing in [0, 1]          */
    bool  voiced;     /* true when the HMM decodes to a voiced pitch state    */
} PYINResult;

/* ── Opaque context ──────────────────────────────────────────────────────── */

typedef struct PYINContext PYINContext;

typedef void* (*allocfn_t)(void *p, size_t num);
typedef void (*freefn_t)(void *p, void *mem);


/* ── API ────────────────────────────────────────────────────────────────── */

/**
 * Allocate and initialise a pYIN context for the given configuration.
 * Returns NULL if any constraint is violated or a memory allocation fails.
 */
PYINContext *pyin_create(PYINConfig cfg, allocfn_t allocfn, freefn_t freefn, void *allocdata);

/**
 * Release all resources owned by ctx.  Safe to call with NULL.
 */
void pyin_destroy(PYINContext *ctx);

/**
 * Return a read-only pointer to the configuration stored in ctx.
 */
const PYINConfig *pyin_get_config(const PYINContext *ctx);

/**
 * Push exactly cfg.block_size mono float samples (normalised to −1…+1).
 *
 * Once the ring-buffer holds a full frame and cfg.hop_size new samples have
 * arrived since the last analysis, one frame is processed and *result is set.
 *
 * Returns true  → *result holds a fresh pitch estimate.
 * Returns false → more samples needed; *result is unchanged.
 */
bool pyin_process_block(PYINContext   *ctx,
                        const float   *samples,  /* length == cfg.block_size */
                        PYINResult    *result);

/**
 * Reset all internal state (ring-buffer, HMM trellis, hop counter).
 * Configuration is preserved.
 */
// void pyin_reset(PYINContext *ctx);

#endif /* PYIN_H */
