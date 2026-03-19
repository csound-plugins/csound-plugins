<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

/* Example file for pvsentropy

ktotalmag pvsentropy fsig, kminfreq=0, kmaxfreq=sr/2

This gives a measure which ranges from approx 0 for a pure sinusoid, 
to ~180 for white noise, a measure of general peakiness of the spectral 
distribution

*/

instr 1
  asig1 = oscili:a(0.5, 500)
  asig2 = buzz(0.1, 300, 7, -1)
  asig3 = pinker() * 0.1
  asig4 = unirand:a(2) - 1.0
  asig5 = diskin2("finnegan01.flac", 1, 0, 1)[0]
  Snames[] fillarray "sine ", "buzz ", "pink ", "white", "finn "
  ksource init 0
  if metro(0.5) == 1 then
    ksource = (ksource + 1) % 5
  endif
  ifftsize = 1024
  asig = picksource(ksource, asig1, asig2, asig3, asig4, asig5)
  fsig = pvsanal(asig, ifftsize, 256, ifftsize, 0)
  kentr = pvsentropy(fsig, 50)
  kentrnorm = limit(((kentr - 0.55) / 180) ^ 0.6, 0, 1)
  ; kentrnorm = bpf(kentr, 0, 0, 0.55, 0, 6, .15, 20, .5, 60, .9, 180, 1.) 
  if metro(20) == 1 then
    printsk "Source: %d, %s, entropy: %.2f (%d %%)\n", ksource, Snames[ksource], kentr, kentrnorm*100
  endif
  outch 1, asig
endin

</CsInstruments>

<CsScore>
i1 0 20

</CsScore>
</CsoundSynthesizer>
