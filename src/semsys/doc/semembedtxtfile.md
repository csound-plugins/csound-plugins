# semembedtxtfile

## Abstract

Embed the text contents of a file at init, returning one embedding per chunk.

## Description

`semembedtxtfile` reads the whole file at `path` and embeds it with the end-to-end model
(the model tokenizes internally), returning a **2D array `[nchunks, ldim]`**. It is the
file counterpart of the i-rate [semembedtxt](semembedtxt.md): text longer than the model
window is split into `≤`-window token chunks (the same chunker as
[semspacebuild](semspacebuild.md)), producing one mean-pooled embedding **row per chunk**.
A short file yields a single row.

The whole file is treated as one block of text (paragraph/blank-line structure is not
significant); chunking is by token window. Embedding runs **once at init** (i-rate).

## Syntax

```csound
chunks:i[][] = semembedtxtfile(handle:i, path:S)
```

## Arguments

* `handle:i`: handle returned by [semload](semload.md).
* `path:S`: path to a UTF-8 text file.

## Output

* `chunks:i[][]`: `[nchunks, ldim]` — one mean-pooled embedding per chunk.

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
    emb:i[][] = semembedtxtfile(handle, "notes.txt")
    prints("chunks=%d  dim=%d\n", lenarray(emb), lenarray(emb, 2))
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semload](semload.md)
* [semdim](semdim.md)
* [semembedtxt](semembedtxt.md)
* [semspacequerytxt](semspacequerytxt.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
