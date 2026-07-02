# semspacequerytxt

## Abstract

Find the nearest stored vectors to a query text (k-nearest neighbours).

## Description

`semspacequerytxt` embeds `query` with the **text model** passed as `handle`, normalizes it,
and compares it against every vector in the space (opened with [semspace](semspace.md)) by
**cosine similarity**. It returns the `top-k` closest entries, sorted from most to least
similar.

The model is chosen **per call** — the space holds no model, only vectors. `handle` must be
a **text** model whose dim equals the space's dim. To query the same space with audio, use
[semspacequeryaudio](semspacequeryaudio.md).

A query longer than the model window is not truncated: it is split into `≤`-window token
chunks, each chunk is embedded, and the chunk embeddings are **mean-pooled** into one
centroid query vector — so the whole query text contributes. (Short queries embed to a
single vector.)

Search is **brute force**: a linear scan over the whole space, in RAM, with a bounded
min-heap keeping only the `top-k` best. `top-k` is clamped to the number of stored vectors.
The result is the matching **vectors and scores, not the source text** (see
[semspace](semspace.md)).

Two forms, selected by the **rate of the output variables**:

* **i-rate** (`i[][]`, `i[]` outputs) — query **once at init**.
* **k-rate** (`k[][]`, `k[]`, `k` outputs) — query in real time, **self-gated**:
  re-embeds and re-searches only when `query` changes. `kgate` pulses to 1 when a fresh
  worker result has been published, else it is 0. Work runs on a per-instance background
  thread with latest-wins scheduling: if query changes arrive faster than processing,
  intermediate jobs are discarded and stale results are not published.

## Syntax

```csound
neighs:i[][], scores:i[] = semspacequerytxt(space:i, handle:i, query:S, topk:i)   ; i-rate
neighs:k[][], scores:k[], kgate:k = semspacequerytxt(space:i, handle:i, query:S, topk:i)   ; k-rate
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `handle:i`: a **text** embedding model from [semload](semload.md) (dim must match the space).
* `query:S`: the query text.
* `topk:i`: number of neighbours to return (clamped to the space size).

## Output

* `neighs[][]`: nearest vectors (`topk × dim`).
* `scores[]`: their cosine similarity scores (length `topk`, descending).
* `kgate:k`: k-rate form only. `1` for the k-pass where a fresh async result is ready,
  otherwise `0`.

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
