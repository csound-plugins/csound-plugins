# semdim

## Abstract

Return the embedding dimension of a loaded model.

## Description

`semdim` returns the dimensionality of the embedding vectors produced by the model
loaded with [semload](semload.md). The value is detected automatically from the
model output at load time (for example, `384` for `sentence-transformers/all-MiniLM-L6-v2`).

Use it to size arrays that hold embeddings, or to verify the model you loaded.

## Syntax

```csound
dim:i = semdim(handle:i)
```


## Arguments

* `handle:i`: handle returned by [semload](semload.md).

## Output

* `dim:i`: the embedding (latent) dimension.

## Execution Time

* Init

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semdim.csd
;
; semdim reports the embedding (latent) dimension of a loaded model. Use it to
; size the arrays that hold embeddings, or to verify the model you loaded.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
0dbfs = 1

#define MODEL_DIR # "path/to/text_model_dir" #

handle@global:i = semload(256, $MODEL_DIR)

instr 1
    ldim:i = semdim(handle)
    prints("latent dimension = %d\n", ldim)
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semload](semload.md)
* [semembedtxt](semembedtxt.md)
* [semembedaudio](semembedaudio.md)

## Credits

Pasquale Mainolfi, 2026
