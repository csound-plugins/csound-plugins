<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
  asig oscili 0.5, 1000
  aenv linseg 0, 0.1, 1, 1.9, 1, 1, 0 ; total 0.1+1.9+1=3
  asig *= aenv
  kfinished = detectsilence(asig, db(-90), 0.1)
  if kfinished == 1 then
  	println ">>> Silence detected at %.3f", timeinsts()
    turnoff
  endif
endin

</CsInstruments>

<CsScore>

i1 1 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6
i1 + 6


; f0 3600

</CsScore>
</CsoundSynthesizer>
