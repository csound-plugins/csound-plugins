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

instr 4
  iA[] fillarray 0, 1, 2, 3, 4, 5
  iB[] fillarray 10, 20, 30
  irefs[] fillarray ref(iA), ref(iB)
  kidx init 0
  if metro(20) == 1 then
    kidx = (kidx + 1) % 2
  endif
  karr[] deref irefs[kidx]
  if changed(kidx) == 1 then
    println "kidx: %d", kidx
    printarray karr
  endif

endin


</CsInstruments>

<CsScore>

; i1 0 0.1

i4 0 0.5
f0 100
; f0 3600

</CsScore>
</CsoundSynthesizer>
