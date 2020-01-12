# sref

## Abstract

Create a reference to a string or retrieve a string from a reference


## Description

`sref` implements a global string cache. This can be useful in situations where
a string needs to be stored/passed but only numbers are allowed (for example,
when using the `event` opcode). It behaves similar to the `strset` / `strget`
opcodes but automatically assigns an index to each distinct string inside the
cache and guarantees that passing twice the same string will return the same
index.

`sref` executes both at **i-time** and **k-time**, depending on the type of
the input variable

!!! Note

    The same opcode is used for creating a reference to a string and from
    converting that reference back to a string.

## Syntax

    Sstr sref idx
    Sstr sref kdx
    idx sref Sstr
    kdx sref Sstr

### Arguments

* `idx` / `kdx`: the numeric id representing the string
* `Sstr`: the string inside the cache, corresponding to `idx`

### Execution Time

* Init
* Performance

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

/*
Example file for sref

*/

; Use sref to pass multiple strings between instruments
instr 1
  event_i "i", 2, 0, -1, sref("foo"), sref("bar")
  turnoff
endin

instr 2
  S1 sref p4
  S2 sref p5
  prints "S1=%s   S2=%s \n", S1, S2
  turnoff
endin

;; Use sref to store strings inside a numeric array
instr 3
  iStruct[] fillarray sref("Bach"), 1675, 1750
  prints "Name = %s\n", sref(iStruct[0])
endin
  
</CsInstruments>

<CsScore>

i 1 0 0.1
i 3 + 0.1

</CsScore>
</CsoundSynthesizer>

```


## Credits

Eduardo Moguillansky, 2019
