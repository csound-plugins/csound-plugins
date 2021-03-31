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

;; If the string does not need to be modified, sderef
;; can be used instead of sref to retrieve a string
;; from the cache. In this case, the string is not allocated,
;; it only points to the version inside the cache. 
instr 4
  S1 = "foo bar"
  iS1 = sref(S1)
  ;; S2 is a read-only view of the cached S1.
  S2 = sderef(iS1)
  prints "S2 = %s \n", S2
  turnoff
endin
  
instr test_same_idx
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