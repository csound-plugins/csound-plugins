# semspaceclear

## Abstract

Clear all vectors from an in-memory semantic space.

## Description

`semspaceclear` empties a [semspace](semspace.md) handle by setting its stored vector
count to zero. The space remains valid, keeps its embedding dimension, and can be populated
again with [semspaceaddtxt](semspaceaddtxt.md) or [semspaceaddaudio](semspaceaddaudio.md).

The allocated vector capacity is kept for reuse; the opcode does not free and recreate the
space. It also does not change or unload the embedding model handle.

The k-rate form clears on a **rising edge** of `ktrig` (`<= 0` to `> 0`). Holding `ktrig`
positive does not clear repeatedly.

No synchronization is performed with asynchronous k-rate queries already running on the
same space. For deterministic results, trigger `semspaceclear` when no pending query on
that space matters.

## Syntax

```csound
semspaceclear(space:i)
semspaceclear(space:i, trig:k)
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `trig:k`: clear on rising edge.

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
; semspaceclear.csd
;
; semspaceclear empties a space (vector count -> 0) while keeping its dimension and
; allocated capacity, so it can be repopulated. Here: add + query, clear, then
; query again -- the match score collapses after the clear.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define MODEL_DIR # "path/to/text_model_dir" #

e_handle@global:i = semload(256, $MODEL_DIR)
s_handle@global:i = semspace(e_handle)

instr ADD
    semspaceaddtxt(s_handle, e_handle, "a warm analog texture")
    semspaceaddtxt(s_handle, e_handle, "a bright metallic hit")
    prints("space populated\n")
    turnoff
endin

instr QUERY_BEFORE
    neighs:i[][], scores:i[] = semspacequerytxt(s_handle, e_handle, "warm analog sound", 1)
    prints("before clear score = %.6f\n", scores[0])
    turnoff
endin

instr CLEAR
    semspaceclear(s_handle)
    prints("space cleared\n")
    turnoff
endin

instr QUERY_AFTER
    neighs:i[][], scores:i[] = semspacequerytxt(s_handle, e_handle, "warm analog sound", 1)
    prints("after clear score = %.6f\n", scores[0])
    turnoff
endin

</CsInstruments>
<CsScore>
i "ADD"          0 0.1
i "QUERY_BEFORE" 1 0.1
i "CLEAR"        2 0.1
i "QUERY_AFTER"  3 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspace](semspace.md)
* [semspaceaddtxt](semspaceaddtxt.md)
* [semspaceaddaudio](semspaceaddaudio.md)
* [semspacesave](semspacesave.md)

## Credits

Pasquale Mainolfi, 2026
