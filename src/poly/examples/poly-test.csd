<CsoundSynthesizer>
<CsOptions>
; -odac           

</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1


; multiple oscillators, mixed down to mono
instr 1

  ; number of oscillators
  inum = 10
  
  ; fundamental
  kmidi = line(ntom:i("3G"), p3, ntom:i("3C"))
  kf0 = mtof(kmidi)

  ; each oscillator is an overtone of f0
  kRatios[] genarray_i 1, inum
    
  ; harmonicity over time
  kexp line 1, p3, 1.32

  ; the freq. of each oscillator
  kFreqs[] = (kRatios ^ kexp) * kf0

  ; generate the oscillators. 
  aOscils[] poly inum, "oscili", 1/inum, kFreqs

  amono sumarray aOscils
  amono *= 0.1
  outs amono, aOscils[0]
endin

instr 2
  asig oscili 1, 1000
  asigs[] init 2
  asigs[0] = oscili:a(1, 1000)
  asigs[1] = oscili:a(1, 1500)
  aout[] poly 2, "testopc", asigs, 0.1
  ; atest _testopc asig, 0.1
  ; asum = sumarray(aout)
  ; outch 1, asum
  ; outch 1, aout[0], 2, aout[1]
endin    

</CsInstruments>
<CsScore>

i 2 0 8

</CsScore>
</CsoundSynthesizer>
