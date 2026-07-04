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

; -----------------------------------------------------------------------------
; semspace.csd
;
; semspace creates an in-memory vector space (pass no path for an empty RAM space,
; or a .espc file / directory / array to load one). The space is a pure vector
; store: the embedding model is passed per add/query. Here: add sentences, save
; the space to disk, then query it.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

// load the end-to-end embedding model (dir must contain model.onnx; keep model.onnx.data there too if present)
e_handle@global:i = semload(256, "path/to/model_dir")

// empty in-memory space (RAM-only). Use semspace(e_handle, "space.espc") to load an existing one.
s_handle@global:i = semspace(e_handle)

instr 1 // add sentences to the space (i-rate form: each call embeds + appends once at init)
    semspaceaddtxt(s_handle, e_handle, "deep blue ocean under a calm evening sky")
    semspaceaddtxt(s_handle, e_handle, "a red sunset burns over the marine horizon")
    semspaceaddtxt(s_handle, e_handle, "rockets roar as they climb into orbit")
    semspaceaddtxt(s_handle, e_handle, "green forests breathe in the cold morning mist")
    semspaceaddtxt(s_handle, e_handle, "a lone violin sings a slow melancholic tune")
    turnoff
endin

instr 2 // persist the in-memory space to disk (scheduled after instr 1, so it sees all adds)
    semspacesave(s_handle, "space.espc")
    turnoff
endin

instr 3 // query the space
    top_k:i = 3
    query:S = "blue marine in red sky"
    neighs:k[][], scores:k[], kgate:k = semspacequerytxt(s_handle, e_handle, query, top_k)
endin

</CsInstruments>
<CsScore>
i 1 0 4
i 2 4 0.1
i 3 5 1
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

Pasquale Mainolfi, 2026
