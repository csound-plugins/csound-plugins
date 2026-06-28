# semspacequery

## Abstract

Find the nearest stored vectors to a query text (k-nearest neighbours).

## Description

`semspacequery` embeds `query`, normalizes it, and compares it against every vector in the space (opened with [semspace](semspace.md)) by **cosine similarity**. It returns the `top-k` closest entries, sorted from most to least similar.

Search is **brute force**: a linear scan over the whole space. `top-k` is clamped to the number of stored vectors.

It is **self-gated**: the query is re-embedded and the search re-run only when `query` changes; otherwise the previous outputs are held.

Because the space stores vectors only, the result is the matching **vectors and scores, not the source text** (see [semspace](semspace.md)). A tokenizer must have been provided to [semload](semload.md).

The scan runs entirely in RAM (no disk access), and a bounded min-heap keeps only the `top-k` best as it goes. Thanks to the self-gate, the search runs only when `query` changes, not on every k-cycle.

## Syntax

```csound
neighs:k[][], scores:k[] = semspacequery(space:i, query:S, topk:i)
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `query:S`: the query text (k-rate string).
* `topk:i`: number of neighbours to return (clamped to the space size).

## Output

* `neighs:k[][]`: nearest vectors (`topk × dim`).
* `scores:k[]`: their cosine similarity scores (length `topk`, descending).

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

## See also

* [semspace](semspace.md)
* [semspaceadd](semspaceadd.md)
* [semspacebuild](semspacebuild.md)
* [semembed](semembed.md)

## Credits

Pasquale Mainolfi, 2026
