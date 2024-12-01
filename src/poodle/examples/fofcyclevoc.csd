<CsoundSynthesizer>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
  kgate = trighold:k(metro(4), 0.5)
  kpitch = 60
  asig fofcycle kgate, mtof:k(kpitch), 0.9, "vowel", 0
  outch 1, asig
endin  

</CsInstruments>

<CsScore>
i 1 0 10


</CsScore>
</CsoundSynthesizer>
