# semspacequery

## Abstract

Find the nearest stored vectors to a query text (k-nearest neighbours).

## Description

`semspacequery` embeds `query`, normalizes it, and compares it against every vector in the space (opened with [semspace](semspace.md)) by **cosine similarity**. It returns the `top-k` closest entries, sorted from most to least similar.

A query longer than the model window is not truncated: it is split into `≤`-window token chunks, each chunk is embedded, and the chunk embeddings are **mean-pooled** into one centroid query vector — so the whole query text contributes to the search. (Short queries embed to a single vector, unchanged.)

Search is **brute force**: a linear scan over the whole space, in RAM (no disk access), with a bounded min-heap keeping only the `top-k` best as it goes. `top-k` is clamped to the number of stored vectors.

Because the space stores vectors only, the result is the matching **vectors and scores, not the source text** (see [semspace](semspace.md)). An embedding model must have been loaded with [semload](semload.md).

Two forms, selected by the **rate of the output variables**:

* **i-rate** (`i[][]`, `i[]` outputs) — query **once at init**. Use it when the query
  string is fixed for the note.
* **k-rate** (`k[][]`, `k[]` outputs) — query in real time, **self-gated**: re-embeds
  and re-searches only when `query` changes, else holds the previous outputs.

## Syntax

```csound
neighs:i[][], scores:i[] = semspacequery(space:i, query:S, topk:i)   ; i-rate, once
neighs:k[][], scores:k[] = semspacequery(space:i, query:S, topk:i)   ; k-rate, real-time
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `query:S`: the query text.
* `topk:i`: number of neighbours to return (clamped to the space size).

## Output

* `neighs[][]`: nearest vectors (`topk × dim`).
* `scores[]`: their cosine similarity scores (length `topk`, descending).

## Execution Time

* Init (i-rate form)
* Init + Performance (k-rate form)

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

e_handle@global:i = semload(256, "path/to/model_dir")
s_handle@global:i = semspace(e_handle, "space.espc")

instr 1
    top_k:i = 3
    query:S = "blue marine in red sky"
    neighs:k[][], scores:k[] = semspacequery(s_handle, query, top_k)
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
* [semspaceadd](semspaceadd.md)
* [semspacebuild](semspacebuild.md)
* [semembed](semembed.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
