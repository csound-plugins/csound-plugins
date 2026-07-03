# semspacequerytxt

## Abstract

Find the nearest stored vectors to a query text (k-nearest neighbours).

## Description

`semspacequerytxt` embeds `query` with the **text model** passed as `handle`, normalizes it,
and compares it against every vector in the space (opened with [semspace](semspace.md)) by
**cosine similarity**. It returns the `top-k` closest entries, sorted from most to least
similar.
The model is chosen **per call**. The space holds no model, only vectors. `handle` must be
a **text** model whose dim equals the space's dim. To query the same space with audio, use
[semspacequeryaudio](semspacequeryaudio.md).

A query longer than the model window is not truncated: it is split into `<=` window token
chunks, each chunk is embedded, and the chunk embeddings are **mean-pooled** into one
centroid query vector, so the whole query text contributes. (Short queries embed to a
single vector.)

## Syntax

```csound
neighs:i[][], scores:i[] = semspacequerytxt(space:i, handle:i, query:S, topk:i)   ; i-rate
neighs:k[][], scores:k[], kgate:k = semspacequerytxt(space:i, handle:i, query:S, topk:i)   ; k-rate
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `handle:i`: a **text** embedding model from [semload](semload.md) (dim must match the space).
* `query:S`: the query text.
* `topk:i`: number of output neighbour slots to return. Must be greater than `0`.

## Output

* `neighs[][]`: nearest vectors (`topk × dim`).
* `scores[]`: their cosine similarity scores (length `topk`, descending). If the space
  contains fewer than `topk` matches, the remaining rows/scores stay zero.
* `kgate:k`: k-rate form only. `1` for the k-pass where a fresh async result is ready,
  otherwise `0`.

## Execution Time

* Init
* Init + Performance

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

e_handle@global:i = semload(256, "path/to/text_model_dir")
s_handle@global:i = semspace(e_handle, "space.espc")

instr 1
    top_k:i = 3
    query:S = "blue marine in red sky"
    neighs:k[][], scores:k[], kgate:k = semspacequerytxt(s_handle, e_handle, query, top_k)
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
```

For full **semantic synthesis** demos driven by a query, see `examples/sem_synthesis.csd`
(query → filter impulse response → `ftconv`) and `examples/sem_synthesis_additive.csd`
(query → additive sine bank).

## See also

* [semspace](semspace.md)
* [semspacequeryaudio](semspacequeryaudio.md)
* [semspaceaddtxt](semspaceaddtxt.md)
* [semspacebuild](semspacebuild.md)
* [semembedtxt](semembedtxt.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
