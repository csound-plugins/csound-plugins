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
  S1 strcache p4
  S2 strcache p5
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
  prints "idx1=%d, idx2=%d \n", idx1, idx2
  turnoff
endin
</CsInstruments>

<CsScore>

; i 1 0 0.1
; i 3 + 0.1
i 4 + 0.1
; i "test_same_idx" 0 1
</CsScore>
</CsoundSynthesizer>