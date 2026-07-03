# semembedaudio

## Abstract

Embed live a-rate audio into a semantic vector, computed on a background worker thread.

## Description

`semembedaudio` accumulates the incoming a-rate signal `asig` into fixed-length **windows**
(`iwindow` seconds, default 10, minimum 1) and runs each full window through an end-to-end
audio embedding model (e.g. **PANNs CNN14**), returning the model's pooled **embedding** as
a 1D array `k[]` of length = embedding dimension (see [semdim](semdim.md)). The vector is
**L2-normalized**. This is audio embedding, not classification. Near-**silent** windows (below an RMS floor) are **skipped**, no inference and no gate pulse.
Load the model with [semload](semload.md). Models with global time pooling impose no
sequence cap. Load them with `maxlen = -1` ("full").
Keep `iwindow` comfortably larger. If you need **every** window with no gaps,
prefer the i-rate [semembedaudiofile](semembedaudiofile.md) / [semembedaudioft](semembedaudioft.md)

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
        check_coeffs:k = sqrt(sum(kemb^2))
        println("fresh audio embedding ready (dim %d) | coeffs check: %.3f", lenarray(kemb), check_coeffs)
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
