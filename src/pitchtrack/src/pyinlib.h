#ifndef PYIN_H
#define PYIN_H

#include <stdint.h>
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
     * The state space spans MIDI notes 21–108 (A0–C8, 88 semitones).
     * Finer grids track vibrato and glides more accurately and reduce
     * Viterbi quantisation error.  The banded Viterbi is O(states × 8σ)
     * per frame so cost scales linearly, not quadratically.
     *
     *   1  → 100 cents/state  (88 states)    coarse, very fast
     *   2  →  50 cents/state  (176 states)
     *   4  →  25 cents/state  (352 states)
     *  10  →  10 cents/state  (880 states)   recommended default
     *  20  →   5 cents/state  (1760 states)  fine
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
     * Voiced observation floor: minimum value clamped onto max_p_voiced
     * before computing the HMM log-observation for voiced states.
     *
     * Without a floor, a single genuinely ambiguous frame (e.g. during a
     * creak, glottalization, or breath) where CMNDF is high can produce
     * max_p_voiced ≈ 0.15, giving log_obs_voiced ≈ −1.9 nats.  This is
     * enough to let the unvoiced path overtake the voiced path, and with
     * the default voiced_transition_weight=0.10 it then takes 8–10 strong
     * frames to recover.
     *
     * Setting voiced_obs_floor=0.20 means: "even if the CMNDF looks very
     * bad for one frame, treat the voiced probability as at least 0.20
     * rather than committing fully to unvoiced."  This bounds the debt a
     * single bad frame can impose on the voiced path.
     *
     * Interpretation of values:
     *   0.00  – disabled; bad frames impose full cost on voiced path (default)
     *   0.10  – mild smoothing; only helps with very weak evidence frames
     *   0.20  – recommended for natural speech with occasional creaky voice,
     *           breathiness, or microphone noise
     *   0.30  – aggressive; may prevent detection of genuinely unvoiced
     *           short segments embedded in voiced speech
     *
     * This parameter only affects the observation floor; it does NOT affect
     * the energy gate (silence is still handled separately).
     *
     * NOTE: voiced_obs_floor is most effective in combination with a low
     * voiced_transition_weight (0.01–0.05).  With a high weight, the
     * Viterbi switches state too easily for the floor to help.
     *
     * Default: 0.0 (disabled)
     */
    float voiced_obs_floor;  /* Default: 0.0f */

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
 *   octave_cost_weight        = 0.0
 *   octave_subharmonic_threshold = 3.0
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
