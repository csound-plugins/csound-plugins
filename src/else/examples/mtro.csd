<CsoundSynthesizer>
<CsOptions>
-odac 
--nosound
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 64


opcode gettime, k, 0
  xout (timeinstk:k() - 1) * (ksmps/sr)
endop

instr 1
  ktrig mtro 7
  kt = gettime()
  if ktrig == 1 then
    println "triggered! time: %f", kt
  endif
endin

instr 2
  ktrig metro 7
  kt = gettime()
  if ktrig == 1 then
    println "triggered! time: %f", kt
  endif
endin


</CsInstruments>
<CsScore>
i 1 0 20
; i 2 0 20

</CsScore>
</CsoundSynthesizer>
