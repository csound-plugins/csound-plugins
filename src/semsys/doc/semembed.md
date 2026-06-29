# semembed

## Abstract

Embed a text string in real time, returning its sentence and token embeddings.

## Description

`semembed` tokenizes `text` and runs it through the embedding model, producing the
**mean-pooled sentence embedding** (a single vector, length = embedding dimension, see
[semdim](semdim.md)) and the **per-token embeddings** (a matrix `maxlen × dim`, one row
per token position, unused positions zero-padded). A tokenizer must have been provided
to [semload](semload.md), otherwise `semembed` raises an error. The text is truncated
to the model/tokenizer limits.

It comes in two forms, selected automatically by the **rate of the output variables**:

* **i-rate** (`i[]`, `i[][]` outputs) — embed **once at init**. Two outputs, no change
  flag. Use it to compute a fixed embedding for the note.
* **k-rate** (`k[]`, `k[][]`, plus a `k` flag) — embed in real time, **self-gated**:
  the model re-runs only when the text changes, and `changed` is `1` on that pass. Use
  it for live latent-space control.

## Syntax

```csound
pool:i[], tokens:i[][]            = semembed(handle:i, text:S)   ; i-rate, once
pool:k[], tokens:k[][], changed:k = semembed(handle:i, text:S)   ; k-rate, real-time
```

## Arguments

* `handle:i`: handle returned by [semload](semload.md) (with a tokenizer).
* `text:S`: the text to embed.

## Output

* `pool[]`: mean-pooled sentence embedding (length = embedding dim).
* `tokens[][]`: per-token embeddings (`maxlen × dim`).
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
    pool_embed:k[], tokens_embed:k[][], changed:k = semembed(handle, sentence)
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
* [semspacequery](semspacequery.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
