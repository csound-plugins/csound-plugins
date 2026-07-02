# semspacesave

## Abstract

Write the in-memory semantic space to a `.espc` file.

## Description

`semspacesave` dumps the whole space (its header plus every vector) to the file at `file`. The space lives in RAM (see [semspace](semspace.md)); `semspacesave` is how you persist it. Each call writes a **full snapshot** of the current state and **overwrites** `file` — it is not incremental. Writing to the same path always leaves a single, up-to-date file; using a different path each time produces independent snapshots.

The destination is always a **new** file: it is unrelated to any cache file passed to `semspace`, which is never modified.

Two forms:

* `semspacesave(space:i, file:S)` — saves once, at init.
* `semspacesave(space:i, file:S, trig:k)` — saves during performance, on the **rising edge** of `trig` (`trig > 0` while the previous value was `<= 0`). It does not write on every k-cycle.

Because the i-time form runs at note init, while [semspaceaddtxt](semspaceaddtxt.md) appends at performance time, schedule the saving instrument **after** the one that fills the space (or use the k-form on a trigger).

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

* Init (`semspacesave(space, file)`)
* Performance (`semspacesave(space, file, trig)`)

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
s_handle@global:i = semspace(e_handle)   ; RAM-only space

; instr 1 fills the space during its performance
instr 1
    text:S, line:k = readf("space_text.txt")
    if (line == -1) then
        turnoff
    endif
    semspaceaddtxt(s_handle, e_handle, text)
endin

; instr 2 is scheduled later, so its i-time save sees all the adds
instr 2
    semspacesave(s_handle, "space.espc")
    turnoff
endin

; alternatively, save on a trigger during performance
instr 3
    ktrig = metro(0.5)
    semspacesave(s_handle, "space.espc", ktrig)
endin

</CsInstruments>
<CsScore>
i 1 0 4
i 2 4 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspace](semspace.md)
* [semspaceaddtxt](semspaceaddtxt.md)
* [semspacebuild](semspacebuild.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
