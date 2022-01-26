<CsoundSynthesizer>
<CsOptions>
-odac           
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

/* Example file for zerocrossing

afreq zerocrossing asig

  Outputs a frequency based upon the distance between interceptions of the X axis.
  The X intercepts are determined via linear interpolation so this gives better 
  than just integer wavelength resolution. This is a very crude pitch follower, 
  but can be useful in some situations.
  
  zerocrossing is a port of supercollider's ZeroCrossing ugen
*/

instr 1
  kfreq line 500, p3, 600
  asin = oscili:a(0.1, kfreq)
  anoise = pinker()
  ksource = bpf(timeinsts(), 0, 0, p3*0.5, 0, p3*0.51, 1, p3, 1)
  asig = asin*(1-ksource) + anoise * ksource  
  acrossings zerocrossing asig
  kcrossings = k(acrossings)
  aresynth = oscili:a(0.1, kcrossings)
  kisnoise = lagud(kcrossings > 1000 ? 1 : 0, 0.001, 0.1) 
  outs asig, aresynth * (1 - kisnoise)
  
  println "kfreq: %.1f, zero-crossings: %.1f, noise detected: %d", kfreq, kcrossings, kisnoise
endin

</CsInstruments>

<CsScore>
i1 0 3

</CsScore>
</CsoundSynthesizer>
