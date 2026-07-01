# semspaceadd

## Abstract

Embed a sentence and append its vector(s) to a semantic space.

## Description

`semspaceadd` embeds `sentence` (the model tokenizes internally), normalizes, and appends the vector to the **in-memory** space opened with [semspace](semspace.md). The add is RAM-only; nothing is written to disk. To persist the space, call [semspacesave](semspacesave.md).

Text longer than the model window is **chunked** (the same token-window chunker as [semspacebuild](semspacebuild.md)) and **each chunk is added as a separate entry** — so a long sentence grows the space by several vectors, not one. Short text adds a single vector. (To bulk-build a space from a file or a directory of `.txt`, use [semspacebuild](semspacebuild.md).)

A **consecutive self-gate** skips the add — before embedding — when `sentence` is unchanged from the previous add, so a repeated k-rate trigger with the same text does not re-embed or re-append.

An embedding model must have been loaded with [semload](semload.md). Empty input adds nothing.

Two forms, distinguished by whether a trigger is given:

* **i-rate** (`semspaceadd ispace, Ssentence`) — add **once, at init**. To grow a
  space, issue several calls (e.g. one per scheduled instrument).
* **k-rate** (`semspaceadd ispace, Ssentence, ktrig`) — add during performance, on the
  **rising edge** of `ktrig` (`ktrig > 0` while the previous value was `<= 0`). Use it
  to capture sentences live, on demand.

## Syntax

```csound
semspaceadd(space:i, sentence:S)           ; i-rate, once at init
semspaceadd(space:i, sentence:S, trig:k)   ; k-rate, on rising edge of trig
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `sentence:S`: the text to embed and append.
* `trig:k`: k-rate form only. A vector is added on the rising edge of this signal.

## Execution Time

* Init (`semspaceadd ispace, Ssentence`)
* Performance (`semspaceadd ispace, Ssentence, ktrig`)

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
s_handle@global:i = semspace(e_handle)          // empty in-memory space

instr build
    semspaceadd(s_handle, "a warm analog texture")
    semspaceadd(s_handle, "a bright metallic hit")
    semspaceadd(s_handle, "a deep resonant drone")
    semspacesave(s_handle, "space.espc")          // persist to disk
    turnoff
endin

</CsInstruments>
<CsScore>
i "build" 0 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspace](semspace.md)
* [semspacebuild](semspacebuild.md)
* [semspacequery](semspacequery.md)
* [semspacesave](semspacesave.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
