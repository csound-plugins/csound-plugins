# sref

## Abstract

Insert a string into a global registry and return a reference to it

## Description

`sref` implements a global string cache. This can be useful in any situation 
where a string needs to be stored/passed but only numbers are allowed (for
example, when using the `event` opcode, or to be able to mix numbers and strings
inside an array). It behaves similar to the `strset` / `strget` opcodes but
automatically assigns an index to each distinct string inside the cache. 

To retrieve an internalized string, use `sderef`. The returned string should
not be modified since it is a "view" of the original string. 


!!! Note

    It is guaranteed that passing twice the same string will return the same
    index.

## Syntax

```csound
    
    idx  sref Sstr
    kdx  sref Sstr
```

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

Example file for sref / sderef

*/

; Use sref to pass multiple strings between instruments
instr 1
  event_i "i", 2, 0, -1, sref("foo"), sref("bar")
  turnoff
endin

instr 2
  ;; get a read-only string from the cache
  S1 sderef p4
  S2 sderef p5
  prints "S1=%s   S2=%s \n", S1, S2
  turnoff
endin

;; Use sref to store strings inside a numeric array
instr 3
  iStruct[] fillarray sref("Bach"), 1675, 1750
  prints "Name = %s\n", sderef(iStruct[0])
endin

instr 4
  S1 = "foo bar"
  iS1 = sref(S1)
  ;; S2 is a read-only view of the cached S1, it should not be modified
  S2 = sderef(iS1)
  prints "S2 = %s \n", S2
  turnoff
endin
  
instr test_same_idx
  ;; Calling sref with the same string should result in the same index
  idx1 = sref("foo")
  idx2 = sref("foo")
  prints "These indices should be the same: idx1=%d, idx2=%d \n", idx1, idx2
  turnoff
endin

instr test_sderef
  S1 = "uniquestring"
  idx1 = sref(S1)
  Sview = sderef(idx1)
  prints "Sview = '%s' (should be '%s') \n", Sview, S1
  turnoff
endin

</CsInstruments>

<CsScore>

; i 1 0 0.1
; i 3 + 0.1
; i 4 + 0.1
i "test_same_idx" 0 1

; i "test_sderef" 0 1
</CsScore>
</CsoundSynthesizer>


```


## Credits

Eduardo Moguillansky, 2019
