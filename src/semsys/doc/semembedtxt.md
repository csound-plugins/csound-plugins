# semembedtxt

## Abstract

Embed a text string in real time, returning its mean-pooled sentence embedding.

## Description

`semembedtxt` runs `text` through the end-to-end embedding model (the model tokenizes
internally), producing the **mean-pooled sentence embedding**: a vector of length =
embedding dimension (see [semdim](semdim.md)).

It comes in two forms, selected automatically by the **rate of the output variables**:

* **i-rate** (`i[][]` output): embed **once at init**, returning a **2D array
  `[nchunks, ldim]`**. Text longer than the model window is split into `<=` window token
  chunks (the same chunker as [semspacebuild](semspacebuild.md)), producing one embedding
  **row per chunk**; short text yields a single row. Iterate the rows with `lenarray`.
  To embed a text file instead of an inline string, use [semembedtxtfile](semembedtxtfile.md).
* **k-rate** (`k[]`, plus a `k` flag): embed in real time, **self-gated**: the model
  re-runs only when the text changes, and `changed` is `1` on that pass. Returns a **single
  vector**: text longer than the model window is chunked and the chunk embeddings are
  **mean-pooled** into one vector.

## Syntax

```csound
chunks:i[][] = semembedtxt:i(handle:i, text:S)   ; i-rate, once, one row per chunk
pool:k[], changed:k = semembedtxt:k(handle:i, text:S)   ; k-rate, real-time
```

## Arguments

* `handle:i`: handle returned by [semload](semload.md).
* `text:S`: the text to embed.

## Output

* `chunks:i[][]`: `[nchunks, ldim]`, one mean-pooled embedding per chunk.
* `pool:k[]`: mean-pooled sentence embedding (length = embedding dim).
* `changed:k`: `1` on the pass the text changed, else `0`.

## Execution Time

* Init
* Init + Performance

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semembedtxt.csd
;
; semembedtxt turns text into its mean-pooled sentence embedding. Two forms,
; selected automatically by the output rate:
;   k-rate (k[] + changed flag) : real-time, self-gated (re-runs only when the
;                                 text changes); returns one pooled vector.
;   i-rate (i[][])              : once at init; text longer than the model window
;                                 is chunked, one embedding row per chunk.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

#define MODEL_DIR # "path/to/text_model_dir" #

// load the end-to-end embedding model (dir must contain model.onnx; keep model.onnx.data there too if present)
handle@global:i = semload(256, $MODEL_DIR)

instr 1
    ldim:i = semdim(handle) // get latent dimension
    prints("Latent dimension size: %d\n", ldim)
    turnoff
endin

instr SENTENCE_K
    sentence:S = "sound synthesis in blue sky"
    // pool_embed = mean-pooled sentence vector; changed = 1 on the pass the text changes
    pool_embed:k[], changed:k = semembedtxt:k(handle, sentence)
    printarray(pool_embed, -1)
endin

instr SENTENCE_I
    sentence:S = "On a quiet autumn morning, Emma decided to take a different path to work.  \
        Instead of following the busy streets, she wandered through an old park she had       \
        never noticed before. The trees were covered in golden leaves, and the air smelled    \
        fresh after the night's rain. As she walked, she discovered a small wooden bench       \
        with a notebook resting on it, filled with short messages written by strangers."

    // longer than the model window -> i-rate returns one row per chunk
    pool_embed:i[][] = semembedtxt:i(handle, sentence)
    printarray(pool_embed)
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
i "SENTENCE_K" 2 1
i "SENTENCE_I" 3 1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semload](semload.md)
* [semdim](semdim.md)
* [semembedtxtfile](semembedtxtfile.md)
* [semembedaudio](semembedaudio.md)
* [semspacequerytxt](semspacequerytxt.md)

## Credits

Pasquale Mainolfi, 2026
