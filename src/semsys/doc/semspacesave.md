# semspacesave

## Abstract

Write the in-memory semantic space to a `.espc` file.

## Description

`semspacesave` dumps the whole space (its header plus every vector) to the file at `file`. The space lives in RAM (see [semspace](semspace.md)); `semspacesave` is how you persist it. Each call writes a **full snapshot** of the current state and **overwrites** `file`. It is not incremental. Writing to the same path always leaves a single, up-to-date file; using a different path each time produces independent snapshots.

## Syntax

```csound
semspacesave(space:i, file:S)
semspacesave(space:i, file:S, trig:k)
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `file:S`: destination `.espc` path (created/overwritten).
* `trig:k`: k-form only. A vector is written on the rising edge of this signal.

## Execution Time

* Init
* Performance

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semspacesave.csd
;
; semspacesave writes a full snapshot of the in-memory space to a .espc file
; (overwrites). Here: fill the space during instr "build", then persist it from
; instr "save" scheduled AFTER it, so the i-time save sees every add.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

#define MODEL_DIR # "path/to/text_model_dir" #

e_handle@global:i = semload(256, $MODEL_DIR)
s_handle@global:i = semspace(e_handle)          // RAM-only space

instr build
    semspaceaddtxt(s_handle, e_handle, "deep blue ocean under a calm evening sky")
    semspaceaddtxt(s_handle, e_handle, "rockets roar as they climb into orbit")
    semspaceaddtxt(s_handle, e_handle, "a lone violin sings a slow melancholic tune")
    turnoff
endin

instr save
    semspacesave(s_handle, "space.espc")
    prints("space saved to space.espc\n")
    turnoff
endin

</CsInstruments>
<CsScore>
i "build" 0 0.1
i "save"  1 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspace](semspace.md)
* [semspaceaddtxt](semspaceaddtxt.md)
* [semspacebuild](semspacebuild.md)

## Credits

Pasquale Mainolfi, 2026
