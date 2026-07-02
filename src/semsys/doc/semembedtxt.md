# semembedtxt

## Abstract

Embed a text string in real time, returning its mean-pooled sentence embedding.

## Description

`semembedtxt` runs `text` through the end-to-end embedding model (the model tokenizes
internally), producing the **mean-pooled sentence embedding**: a vector of length =
embedding dimension (see [semdim](semdim.md)).

It comes in two forms, selected automatically by the **rate of the output variables**:

* **i-rate** (`i[][]` output) — embed **once at init**, returning a **2D array
  `[nchunks, ldim]`**. Text longer than the model window is split into `≤`-window token
  chunks (the same chunker as [semspacebuild](semspacebuild.md)), producing one embedding
  **row per chunk**; short text yields a single row. Iterate the rows with `lenarray`.
  To embed a text file instead of an inline string, use [semembedtxtfile](semembedtxtfile.md).
* **k-rate** (`k[]`, plus a `k` flag) — embed in real time, **self-gated**: the model
  re-runs only when the text changes, and `changed` is `1` on that pass. Returns a **single
  vector**: text longer than the model window is chunked and the chunk embeddings are
  **mean-pooled** into one vector (a k-rate array can't change its row count per pass, so
  the per-chunk 2D form is i-rate only). Use it for live latent-space control.

For **audio** embedding (raw sound → semantic vector, no classification), see
[semembedaudiofile](semembedaudiofile.md), [semembedaudioft](semembedaudioft.md) and
[semembedaudio](semembedaudio.md).

## Syntax

```csound
chunks:i[][]        = semembedtxt:i(handle:i, text:S)   ; i-rate, once, one row per chunk
pool:k[], changed:k = semembedtxt:k(handle:i, text:S)   ; k-rate, real-time
```

## Arguments

* `handle:i`: handle returned by [semload](semload.md).
* `text:S`: the text to embed.

## Output

* `chunks:i[][]`: (i-rate form) `[nchunks, ldim]` — one mean-pooled embedding per chunk.
* `pool:k[]`: (k-rate form) mean-pooled sentence embedding (length = embedding dim).
* `changed:k`: (k-rate form only) `1` on the pass the text changed, else `0`.

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

handle@global:i = semload(256, "path/to/model_dir")

instr 1
    sentence:S = "sound synthesis in blue sky"
    pool_embed:k[], changed:k = semembedtxt:k(handle, sentence)
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
```

For **semantic synthesis** demos that turn embeddings into sound, see
`examples/sem_synthesis.csd` (embedding → filter impulse response → `ftconv`) and
`examples/sem_synthesis_additive.csd` (embedding → additive sine bank).

## See also

* [semload](semload.md)
* [semdim](semdim.md)
* [semembedtxtfile](semembedtxtfile.md)
* [semembedaudio](semembedaudio.md)
* [semspacequerytxt](semspacequerytxt.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
