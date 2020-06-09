<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

opcode add, 0, ii
  iref, ivalue xin
  iarr[] deref iref
  iarr = iarr + ivalue
endop

instr 1
  iX[] fillarray 0, 1, 2, 3, 5, 8, 13
  iref = ref(iX)
  prints "\n----\niref: %d\n", iref
  printarray iX
  add iref, 1
  printarray iX
  prints "------------\n"
  
  turnoff
endin

instr 2
  iX[] fillarray 0, 1, 2, 3, 5, 8, 13
  iref ref iX, 1
  ; iY[] = deref(iref, 1)
  ; printarray iY
  schedule 3, 1, 0.1, iref
  turnoff
endin

instr 3
  iX[] deref p4, 1
  printarray iX, "", "instr 3"
  turnoff
endin


</CsInstruments>

<CsScore>

; i1 0 0.1

i2 0 0.1
f0 100
; f0 3600

</CsScore>
</CsoundSynthesizer>
