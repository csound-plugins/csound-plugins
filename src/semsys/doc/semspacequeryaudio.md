# semspacequeryaudio

## Abstract

Find the nearest stored vectors to a query audio file (k-nearest neighbours).

## Description

`semspacequeryaudio` decodes a **PCM16 WAV** file, embeds it with the **audio model** passed
as `handle`, and compares the result against every vector in the space (opened with
[semspace](semspace.md)) by **cosine similarity**, returning the `top-k` closest entries. It
is the audio counterpart of [semspacequerytxt](semspacequerytxt.md).

The model is chosen **per call** — the space holds no model, only vectors. `handle` must be
an **audio** model whose dim equals the space's dim.

The query file is split into ~10 s windows, each window is embedded, and the per-window
embeddings are **mean-pooled** into one centroid query vector — so the whole file
contributes to the search (a short file embeds to a single vector). `query` holds the
**file path** here.

Search is **brute force**: a linear scan over the whole space, in RAM, with a bounded
min-heap keeping the `top-k` best. `top-k` is clamped to the number of stored vectors. The
result is the matching **vectors and scores**, not any source (see [semspace](semspace.md)).

Two forms, selected by the **rate of the output variables**:

* **i-rate** (`i[][]`, `i[]` outputs) — query **once at init**.
* **k-rate** (`k[][]`, `k[]`, `k` outputs) — real time, **self-gated**: re-embeds and
  re-searches only when the file **path** changes. `kgate` pulses to 1 when a fresh
  worker result has been published, else it is 0. Work runs on a per-instance background
  thread with latest-wins scheduling: if path changes arrive faster than processing,
  intermediate jobs are discarded and stale results are not published.

## Syntax

```csound
neighs:i[][], scores:i[] = semspacequeryaudio(space:i, handle:i, path:S, topk:i)   ; i-rate
neighs:k[][], scores:k[], kgate:k = semspacequeryaudio(space:i, handle:i, path:S, topk:i)   ; k-rate
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `handle:i`: an **audio** embedding model from [semload](semload.md) (dim must match the space).
* `path:S`: path to a PCM16 WAV file to use as the query.
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
ksmps = 32
nchnls = 2
0dbfs = 1

a_handle@global:i = semload(-1, "path/to/audio_model_dir")
s_handle@global:i = semspace(a_handle, "sounds.espc")

instr 1
    ; find the 3 stored sounds most similar to a query sound
    neighs:i[][], scores:i[] = semspacequeryaudio(s_handle, a_handle, "query.wav", 3)
    printarray(scores)
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspace](semspace.md)
* [semspacequerytxt](semspacequerytxt.md)
* [semspacequeryaudioft](semspacequeryaudioft.md)
* [semspaceaddaudio](semspaceaddaudio.md)
* [semembedaudio](semembedaudio.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
July 2026.
