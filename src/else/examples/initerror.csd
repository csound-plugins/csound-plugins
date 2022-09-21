<CsoundSynthesizer>
<CsOptions>
-odac 

</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
  ival = p4
  if ival > 3 then
    initerror sprintf("ival is invalid: %d", ival)
  endif
  prints "Rest of instr 1\n"
  turnoff
endin

</CsInstruments>

<CsScore>

i 1 0 0.1 3
i 1 + 0.1 4
; f0 3600

</CsScore>
</CsoundSynthesizer>
