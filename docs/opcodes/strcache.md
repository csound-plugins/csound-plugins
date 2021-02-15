# strcache

## Abstract

Put a string into the global cache or retrieve a string from the cache


## Description

`strcache` implements a global string cache. This can be useful in situations
where a string needs to be stored/passed but only numbers are allowed (for
example, when using the `event` opcode, or to be able to mix numbers and strings
inside an array). It behaves similar to the `strset` / `strget` opcodes but
automatically assigns an index to each distinct string inside the cache.

`strcache` executes both at **i-time** and **k-time**, depending on the type of
the input variable

!!! Note

    It is guaranteed that passing twice the same string will return the same
    index.

!!! Note

    The same opcode is used for creating a reference to a string and from
    converting that reference back to a string.

## Syntax

    Sstr strcache idx
    Sstr strcache kdx
    idx  strcache Sstr
    kdx  strcache Sstr

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

Example file for strcache / strview

*/

; Use strcache to pass multiple strings between instruments
instr 1
  event_i "i", 2, 0, -1, strcache("foo"), strcache("bar")
  turnoff
endin

instr 2
  ;; get a read-only string from the cache
  S1 strview p4
  S2 strview p5
  prints "S1=%s   S2=%s \n", S1, S2
  turnoff
endin

;; Use strcache to store strings inside a numeric array
instr 3
  iStruct[] fillarray strcache("Bach"), 1675, 1750
  prints "Name = %s\n", strcache(iStruct[0])
endin

;; If the string does not need to be modified, strview
;; can be used instead of strcache to retrieve a string
;; from the cache. In this case, the string is not allocated,
;; it only points to the version inside the cache. 
instr 4
  S1 = "foo bar"
  iS1 = strcache(S1)
  ;; S2 is a read-only view of the cached S1.
  S2 = strcache(iS1)
  prints "S2 = %s \n", S2
  turnoff
endin
  
instr test_same_idx
  idx1 = strcache("foo")
  idx2 = strcache("foo")
  prints "These indices should be the same: idx1=%d, idx2=%d \n", idx1, idx2
  turnoff
endin

instr test_strview
  S1 = "uniquestring"
  idx1 = strcache(S1)
  Sview = strview(idx1)
  prints "Sview = '%s' (should be '%s') \n", Sview, S1
  turnoff
endin

</CsInstruments>

<CsScore>

; i 1 0 0.1
; i 3 + 0.1
; i 4 + 0.1
i "test_same_idx" 0 1

; i "test_strview" 0 1
</CsScore>
</CsoundSynthesizer>


```


## Credits

Eduardo Moguillansky, 2019
