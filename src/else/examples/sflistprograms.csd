<CsoundSynthesizer>
<CsOptions>

--nosound
; -m0

</CsOptions>

<CsInstruments>


instr 1
  Sprograms[] sflistprograms "violin.sf3"
  printarray Sprograms
  turnoff
endin

/*
Prints:

"000-000 Campbells Violin", 
"000-001 Campbells V Loop", 
"000-002 Cam's Violin Reverb"
"000-003 Cam's Violin Panned", 
"000-004 Violin- Pan & Reverb", 
"000-005 Cams Violin- tinny"
"000-006 Cams Violin-Vibrato"

*/

</CsInstruments>

<CsScore>

i 1 0 0.1

</CsScore>
</CsoundSynthesizer>
