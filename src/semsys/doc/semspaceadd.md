# semspaceadd

## Abstract

Embed a sentence and append its vector to a semantic space.

## Description

`semspaceadd` tokenizes `sentence`, embeds it (mean-pooled), normalizes the vector and appends it to the **in-memory** space opened with [semspace](semspace.md). The add is RAM-only; nothing is written to disk. To persist the space, call [semspacesave](semspacesave.md).

It is **self-gated**: the model runs and a vector is added only when `sentence` changes from the previous control pass. Feeding a constant string adds one vector and then idles, so a single sustained note adds one entry, not one per k-cycle.

A single sentence is treated as **one** chunk (truncated to `imaxlen` if longer). To build a space from a large text with automatic chunking, use [semspacebuild](semspacebuild.md) instead.

A tokenizer must have been provided to [semload](semload.md). Empty input adds nothing.

## Syntax

```csound
semspaceadd(space:i, sentence:S)
```
## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `sentence:S`: the text to embed and append (k-rate string).

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
s_handle@global:i = semspace(e_handle) // init vector space

; add one line per note from a text file
instr 1
    text:S, line:k = readf("space_text.txt")
    if (line == -1) then
        turnoff
    endif
    semspaceadd(s_handle, text)
endin

</CsInstruments>
<CsScore>
i 1 0 4
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspace](semspace.md)
* [semspacebuild](semspacebuild.md)
* [semspacequery](semspacequery.md)
* [semspacesave](semspacesave.md)

## Credits

Pasquale Mainolfi, 2026
