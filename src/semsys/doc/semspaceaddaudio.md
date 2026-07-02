# semspaceaddaudio

## Abstract

Embed an audio file and append its per-window vector(s) to a semantic space.

## Description

`semspaceaddaudio` decodes a **PCM16 WAV** file, embeds it with the **audio model** passed
as `handle`, normalizes, and appends the vector(s) to the **in-memory** space opened with
[semspace](semspace.md). The add is RAM-only; call [semspacesave](semspacesave.md) to
persist. It is the audio counterpart of [semspaceaddtxt](semspaceaddtxt.md).

The model is chosen **per call** — the space holds no model, only vectors. `handle` must be
an **audio** model whose embedding dim equals the space's dim, otherwise an init error is
raised. Audio and text vectors coexist in one space when their dimensions match.

The audio is split into fixed **~10 s windows** (the model's training length); each window
is embedded into a separate, L2-normalized entry — a long file grows the space by several
vectors. Near-silent windows are skipped. `sentence` holds the **file path** here.

A **consecutive self-gate** skips the add when the path is unchanged from the previous add.
After embedding, vectors already present in the space are skipped, so re-adding content
from an already-loaded `.espc` or from an earlier add does not duplicate entries. Empty
input adds nothing. Only PCM16 WAV is accepted (convert other formats first).

Two forms:

* **i-rate** (`semspaceaddaudio ispace, ihandle, Spath`) — add **once, at init**.
* **k-rate** (`semspaceaddaudio ispace, ihandle, Spath, ktrig`) — add on the **rising edge**
  of `ktrig`.

## Syntax

```csound
semspaceaddaudio(space:i, handle:i, path:S)           ; i-rate, once at init
semspaceaddaudio(space:i, handle:i, path:S, trig:k)   ; k-rate, on rising edge of trig
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `handle:i`: an **audio** embedding model from [semload](semload.md) (dim must match the space).
* `path:S`: path to a PCM16 WAV file to embed and append.
* `trig:k`: k-rate form only. A vector is added on the rising edge of this signal.

## Execution Time

* Init (i-rate form)
* Performance (k-rate form)

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; audio embedder (e.g. PANNs CNN14): -1 = no sequence cap
a_handle@global:i = semload(-1, "path/to/audio_model_dir")
s_handle@global:i = semspace(a_handle)          // empty in-memory space

instr build
    semspaceaddaudio(s_handle, a_handle, "kick.wav")
    semspaceaddaudio(s_handle, a_handle, "snare.wav")
    semspaceaddaudio(s_handle, a_handle, "pad.wav")
    semspacesave(s_handle, "sounds.espc")
    turnoff
endin

</CsInstruments>
<CsScore>
i "build" 0 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspace](semspace.md)
* [semspaceaddtxt](semspaceaddtxt.md)
* [semspacequeryaudio](semspacequeryaudio.md)
* [semembedaudio](semembedaudio.md)
* [semspacesave](semspacesave.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
July 2026.
