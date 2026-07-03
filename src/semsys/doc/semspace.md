# semspace

## Abstract

Create an in-memory semantic vector space, optionally loading `.espc` files.

## Description

`semspace` returns a handle to a **semantic space**: a collection of embedding vectors held **in RAM**. The space is populated with [semspaceaddtxt](semspaceaddtxt.md) / [semspaceaddaudio](semspaceaddaudio.md) or [semspacebuild](semspacebuild.md), searched with [semspacequerytxt](semspacequerytxt.md) / [semspacequeryaudio](semspacequeryaudio.md), and persisted with [semspacesave](semspacesave.md).
The space is a **pure vector store**. The embedding model is passed by the user to each add/query call, so text and audio vectors can share one space as long as they have the same dimension.
The space stores **vectors only**, no source text.

## Syntax

```csound
space:i = semspace(handle:i)
space:i = semspace(handle:i, path:S)
space:i = semspace(handle:i, paths:S[])
```

## Arguments

* `handle:i`: handle returned by [semload](semload.md). Only anchors the space's dimension; add/query bring their own model.
* `path:S`: a `.espc` file, or a directory of `.espc` files, to load into memory (auto-detected). Pass an empty string for an empty RAM-only space.
* `paths:S[]`: an array of `.espc` file paths to load and merge.

## Output

* `space:i`: handle to the semantic space.

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
s_handle@global:i = semspace(e_handle, "space.espc") // init vector space from .espc file (to RAM)

instr 1
    semspaceaddtxt(s_handle, e_handle, "a deep resonant drone")
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspaceaddtxt](semspaceaddtxt.md)
* [semspaceaddaudio](semspaceaddaudio.md)
* [semspacebuild](semspacebuild.md)
* [semspaceclear](semspaceclear.md)
* [semspacequerytxt](semspacequerytxt.md)
* [semspacequeryaudio](semspacequeryaudio.md)
* [semspacequeryaudioft](semspacequeryaudioft.md)
* [semspacesave](semspacesave.md)
* [semload](semload.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
