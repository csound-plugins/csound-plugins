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