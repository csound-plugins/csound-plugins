# semembed

## Abstract

Embed a text string in real time, returning its sentence and token embeddings.

## Description

`semembed` tokenizes `Stext` and runs it through the embedding model, producing: the **mean-pooled sentence embedding**: a single vector (length = embedding dimension, see [semdim](semdim.md)) representing the whole text and the **per-token embeddings**: a matrix of `imaxlen × dim`, one row per token position (unused positions are zero-padded). `semembed` is **stateless and real-time**. Nothing is persisted; it is meant for live latent-space control. The model is re-run **only** when the input text changes, so feeding a constant string costs one inference and then idles. A tokenizer must have been provided to [semload](semload.md), otherwise `semembed` raises an init error. 

The text is truncated to the model/tokenizer limits.

## Syntax

```csound
pool:k[], tokens:k[][], changed:k = semembed(handle:i, text:S)
```

## Arguments

* `handle:i`: handle returned by [semload](semload.md) (with a tokenizer).
* `text:S`: the text to embed (k-rate string).

## Output

* `pool:k[]`: mean-pooled sentence embedding (length = embedding dim).
* `tokens:k[][]`: per-token embeddings (`imaxlen × dim`).
* `changed:k`: `1` on the pass the text changed, else `0`.

## Execution Time

* Init
* Performance

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
    sentence:S = "sound synthesis in blue sky"
    pool_embed:k[], tokens_embed:k[][], changed:k = semembed(handle, sentence)
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semload](semload.md)
* [semdim](semdim.md)
* [semspacequery](semspacequery.md)

## Credits

Pasquale Mainolfi, 2026
