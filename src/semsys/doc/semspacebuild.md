# semspacebuild

## Abstract

Bulk-build a `.espc` semantic space from a text file or a directory of text files.

## Description

`semspacebuild` reads source text, splits it into chunks, embeds each chunk, and writes the resulting vectors to a new `.espc` file. Use it to build a space from a corpus in one pass instead of adding sentences one by one. Load the result later with [semspace](semspace.md).

`source` is auto-detected:

* a **file** → that file is embedded;
* a **directory** → every `.txt` file directly inside it is embedded, in turn, into the same output.

Chunking:

* The source is read **paragraph by paragraph**. A blank line is a hard boundary; a chunk never crosses it.
* Each paragraph is split into **overlapping word-windows** (~`maxlen`-sized words, ~15% overlap). One embedding vector is produced per window.
* Boundaries are **word-based, approximate**, not token-exact. For long inputs to be fully covered, the model's built-in tokenizer must not truncate below `maxlen`, otherwise each window is cut and the tail is lost (see the README, *Model and token limits*).

The output file is **created/overwritten**. The stored vectors carry no source text (see [semspace](semspace.md); the space does not index).

An embedding model must have been loaded with [semload](semload.md).

## Syntax

```csound
semspacebuild(handle:i, dest:S, source:S)
```

## Arguments

* `handle:i`: handle returned by [semload](semload.md).
* `dest:S`: path to the `.espc` file to create (overwritten if present).
* `source:S`: a text file, or a directory of `.txt` files, to embed.

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

e_handle@global:i = semload(256, "path/to/model_dir")

; build "corpus.espc" from a single text file
semspacebuild(e_handle, "corpus.espc", "corpus.txt")

; build "all.espc" from every .txt in a directory
semspacebuild(e_handle, "all.espc", "path/to/texts")

instr 1
    ; load a built space into RAM and query it
    s_handle:i = semspace(e_handle, "corpus.espc")
    neighs:k[][], scores:k[] = semspacequery(s_handle, "warm analog texture", 3)
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
* [semspacequery](semspacequery.md)
* [semspacesave](semspacesave.md)
* [semload](semload.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
