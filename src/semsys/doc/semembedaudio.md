# semembedaudio

## Abstract

Embed live a-rate audio into a semantic vector, computed on a background worker thread.

## Description

`semembedaudio` accumulates the incoming a-rate signal `asig` into fixed-length **windows**
(`iwindow` seconds, default 10, minimum 1) and runs each full window through an end-to-end
audio embedding model (e.g. **PANNs CNN14**), returning the model's pooled **embedding** as
a 1D array `k[]` of length = embedding dimension (see [semdim](semdim.md)). The vector is
**L2-normalized**. This is audio embedding, not classification.

Inference runs on a **per-instance background worker thread** — the same approach as the
speech-to-text opcodes — so the audio thread is **never blocked** and there are no dropouts,
even with a heavy model. The perf pass only accumulates samples (cheap) and hands full
windows to the worker; the worker resamples to the model's rate (32 kHz for PANNs) and runs
the model off the audio thread.

`gate` pulses **`1`** on the single k-pass a fresh embedding is published, and `0`
otherwise; `emb` holds the last embedding until the next one arrives. Poll `gate` to react
only to new vectors. Near-**silent** windows (below an RMS floor) are **skipped** — no
inference, no gate pulse.

Load the model with [semload](semload.md). Models with global time pooling impose no
sequence cap — load them with `maxlen = -1` ("full").

### Latency and coverage

Embeddings arrive **behind real time** by roughly the inference time `T_inf` of one window
(the gate pulses `T_inf` after the window closes, not a full window later). Two regimes:

* **`T_inf < iwindow`** (the normal case: PANNs on ~10 s windows runs in well under 10 s):
  every window is embedded, latency ≈ `T_inf`.
* **`T_inf > iwindow`** (very short windows and/or a slow machine): the worker cannot keep
  up. The design is **latest-wins** — a new full window replaces an un-started one, so the
  published embedding stays **fresh** (bounded latency) but some intermediate windows are
  **skipped** (coverage gaps), rather than building an ever-growing backlog.

Keep `iwindow` comfortably larger than `T_inf`. If you need **every** window with no gaps,
prefer the i-rate [semembedaudiofile](semembedaudiofile.md) / [semembedaudioft](semembedaudioft.md)
paths (which return every window as a row).

## Syntax

```csound
emb:k[], gate:k = semembedaudio(handle:i, asig:a [, iwindow:i])
```

## Arguments

* `handle:i`: handle returned by [semload](semload.md) (an audio embedding model).
* `asig:a`: a-rate audio input (mono, engine sample rate).
* `iwindow:i` (optional): window length in seconds accumulated before each embedding.
  Default `10`, clamped to a minimum of `1`.

## Output

* `emb:k[]`: last L2-normalized audio embedding (length = embedding dim). Zeroed until the
  first window is embedded.
* `gate:k`: `1` on the pass a fresh embedding is published, else `0`.

## Execution Time

* Init + Performance

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; audio embedder: -1 = run full, no sequence cap (e.g. PANNs CNN14)
handle@global:i = semload(-1, "path/to/audio_model_dir")

instr 1
    sig:a = inch(1)                                     ; live mic
    kemb:k[], kgate:k = semembedaudio(handle, sig, 4)   ; 4 s windows
    if (kgate == 1) then
        println("fresh embedding, coeff[0] = %f", kemb[0])
        ; patch kemb into your synthesis: map coords -> params, similarity, ...
    endif
endin

</CsInstruments>
<CsScore>
i 1 0 60
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semload](semload.md)
* [semdim](semdim.md)
* [semembedaudiofile](semembedaudiofile.md)
* [semembedaudioft](semembedaudioft.md)
* [semembedtxt](semembedtxt.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
July 2026.
