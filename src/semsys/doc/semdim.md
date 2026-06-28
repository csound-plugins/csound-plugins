# semdim

## Abstract

Return the embedding dimension of a loaded model.

## Description

`semdim` returns the dimensionality of the embedding vectors produced by the model
loaded with [semload](semload.md). The value is detected automatically from the
model output at load time (for example, `384` for
`sentence-transformers/all-MiniLM-L6-v2`).

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
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

handle@global:i = semload(256, "path/to/model_dir")

instr 1
    ldim:i = semdim(handle)
    prints("Latent dimension size: %d\n", ldim)
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semload](semload.md)
* [semembed](semembed.md)

## Credits

Pasquale Mainolfi, 2026
