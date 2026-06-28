# semspace

## Abstract

Create an in-memory semantic vector space, optionally loading `.espc` files.

## Description

`semspace` returns a handle to a **semantic space**: a collection of embedding vectors held **in RAM**. The space is populated with [semspaceadd](semspaceadd.md) or [semspacebuild](semspacebuild.md), searched with [semspacequery](semspacequery.md), and persisted with [semspacesave](semspacesave.md).

Three forms:

* `semspace(handle:i)` — start an empty space in memory.
* `semspace(handle:i, path:S)` — load from a path. If it is a `.espc` **file**, that file is loaded; if it is a **directory**, every `.espc` inside it is loaded and merged into one space (auto-detected).
* `semspace(handle:i, paths:S[])` — load and merge an array of `.espc` files.

Every loaded file is validated: its magic tags must be valid and its embedding dimension **must match** the model loaded with [semload](semload.md); otherwise an init error is raised. Loading a directory or array **merges** all vectors into a single space — duplicates are **not** removed (it is a union).

Files are read once into RAM and then **closed**; semsys never writes back to them. Adds happen in memory only — to persist, call [semspacesave](semspacesave.md) (which always writes a fresh file). The RAM buffer grows automatically as vectors are added.

The space stores **vectors only**, no source text. It is a brute-force store, not an index: queries scan every vector linearly. Memory and vectors are freed automatically when the instance is deinitialised.

## Syntax

```csound
space:i = semspace(handle:i)
space:i = semspace(handle:i, path:S)
space:i = semspace(handle:i, paths:S[])
```

## Arguments

* `handle:i`: handle returned by [semload](semload.md).
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
    semspaceadd(s_handle, "a deep resonant drone") 
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspaceadd](semspaceadd.md)
* [semspacebuild](semspacebuild.md)
* [semspacequery](semspacequery.md)
* [semspacesave](semspacesave.md)
* [semload](semload.md)

## Credits

Pasquale Mainolfi, 2026
