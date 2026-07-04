# semspaceaddtxt

## Abstract

Embed a text sentence and append its vector(s) to a semantic space.

## Description

`semspaceaddtxt` embeds `sentence` with the **text model** passed as `handle` (the model
tokenizes internally), normalizes, and appends the vector to the **in-memory** space opened
with [semspace](semspace.md). The add is RAM-only; nothing is written to disk. To persist,
call [semspacesave](semspacesave.md).
The model is chosen **per call**. The space itself holds no model, only vectors. `handle`
must be a **text** model whose embedding dim equals the space's dim, otherwise an init error
is raised. To add audio instead, use [semspaceaddaudio](semspaceaddaudio.md); vectors from
both share one space as long as the dimensions match.
Text longer than the model window is **chunked** (the same token-window chunker as
[semspacebuild](semspacebuild.md)) and **each chunk is added as a separate entry**. A long
sentence grows the space by several vectors, not one. Short text adds a single vector.
A **consecutive self-gate** skips the add, before embedding, when `sentence` is unchanged
from the previous add, so a repeated k-rate trigger with the same text does not re-embed.
After embedding, vectors already present in the space are skipped, so re-adding content
from an already-loaded `.espc` or from an earlier add does not duplicate entries. Empty
input adds nothing.

## Syntax

```csound
semspaceaddtxt(space:i, handle:i, sentence:S)           ; i-rate, once at init
semspaceaddtxt(space:i, handle:i, sentence:S, trig:k)   ; k-rate, on rising edge of trig
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `handle:i`: a **text** embedding model from [semload](semload.md) (dim must match the space).
* `sentence:S`: the text to embed and append.
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
; semspaceaddtxt.csd
;
; semspaceaddtxt embeds a sentence with the text model and appends the vector to
; an in-memory space (RAM-only; use semspacesave to persist). Here: fill an empty
; space with a few sentences, then query it to confirm the nearest match.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

#define MODEL_DIR # "path/to/text_model_dir" #

e_handle@global:i = semload(256, $MODEL_DIR)
s_handle@global:i = semspace(e_handle)          // empty in-memory space

instr build
    semspaceaddtxt(s_handle, e_handle, "a warm analog texture")
    semspaceaddtxt(s_handle, e_handle, "a bright metallic hit")
    semspaceaddtxt(s_handle, e_handle, "a deep resonant drone")
    prints("space populated\n")
    turnoff
endin

instr query
    neighs:i[][], scores:i[] = semspacequerytxt(s_handle, e_handle, "warm analog sound", 1)
    prints("best score = %.4f\n", scores[0])
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
* [semspaceaddaudio](semspaceaddaudio.md)
* [semspacebuild](semspacebuild.md)
* [semspacequerytxt](semspacequerytxt.md)
* [semspacesave](semspacesave.md)

## Credits

Pasquale Mainolfi, 2026
