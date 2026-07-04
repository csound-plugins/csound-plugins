# semspaceaddaudio

## Abstract

Embed an audio file and append its per-window vector(s) to a semantic space.

## Description

`semspaceaddaudio` decodes a **PCM16 WAV** file, embeds it with the **audio model** passed
as `handle`, normalizes, and appends the vector(s) to the **in-memory** space opened with
[semspace](semspace.md). The add is RAM-only; call [semspacesave](semspacesave.md) to
persist.
The model is chosen **per call**. The space holds no model, only vectors. `handle` must be
an **audio** model whose embedding dim equals the space's dim, otherwise an init error is
raised. Audio and text vectors coexist in one space when their dimensions match.
A **consecutive self-gate** skips the add when the path is unchanged from the previous add.
After embedding, vectors already present in the space are skipped, so re-adding content
from an already-loaded `.espc` or from an earlier add does not duplicate entries. Empty
input adds nothing. Only PCM16 WAV is accepted (convert other formats first).

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

* Init
* Performance

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semspaceaddaudio.csd
;
; semspaceaddaudio decodes a PCM16 WAV, embeds it with the audio model and appends
; the per-window vector(s) to an in-memory space (RAM-only; use semspacesave to
; persist). Here: add a sound, then query the space with the same sound (its
; self-match score should be close to 1).
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define AUDIO_MODEL # "path/to/audio_model_dir" #
#define WAV # "sound.wav" #

a_handle@global:i = semload(-1, $AUDIO_MODEL)   ; audio model, no sequence cap
s_handle@global:i = semspace(a_handle)          // empty in-memory space

instr build
    semspaceaddaudio(s_handle, a_handle, $WAV)
    prints("audio added to space\n")
    turnoff
endin

instr query
    neighs:i[][], scores:i[] = semspacequeryaudio(s_handle, a_handle, $WAV, 1)
    prints("self-match score = %.4f\n", scores[0])
    turnoff
endin

</CsInstruments>
<CsScore>
i "build" 0 0.1
i "query" 1 0.1
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

Pasquale Mainolfi, 2026
