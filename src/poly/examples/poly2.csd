<CsoundSynthesizer>
<CsOptions>
; -odac           

</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1


; create an array of random values between 0-1
; inum: number of elements in the array
opcode rndarr, i[], i
  inum xin 
  iOut[] init inum 
  i0 = 0
  while i0 < inum do 
    iOut[i0] = unirand(1)
    i0 += 1
  od 
  xout iOut
endop

; multiple oscillators, mixed down to mono
instr 1
  ; number of oscillators
  inum = 100
  
  ; fundamental
  kmidi = line(ntom:i("2G"), p3, ntom:i("1C"))
  kf0 = mtof(kmidi)

  ; each oscillator is an overtone of f0
  kRatios[] genarray_i 1, inum
    
  ; harmonicity over time
  kexp line 1, p3, 1.32

  ; the freq. of each oscillator
  kFreqs[] = (kRatios ^ kexp) * kf0
  println "kexp: %f, kf0: %f", kexp, kf0
  ; array of random phases, to avoid synchronous start
  iPhs[] rndarr inum 

  ; generate the oscillators. 
  aOscils[] poly inum, "oscili", 1/inum, kFreqs, -1, iPhs

  amono sumarray aOscils
  ; amono *= linsegr:a(0, 0.05, 1, 0.05, 0)
  outs amono, amono
endin

; poly instances stacked as a processing pipe
instr 2
  ; amount of polyphony
  inum = 30

  iFreqs[] rndarr inum 
  
  ; the ratios of the overtones
  kRatios[] genarray_i 1, inum
  
  ; a down gliss
  km0 linseg ntom:i("2C"), p3*0.8, ntom:i("1C")
  kf0 = mtof(km0)

  ; harmonicity curve
  kp linsegb 1, p3*0.8, 1.68

  ; calculate actual freqs.
  kFreqs[] = (kRatios ^ kp) * kf0

  ; lfo freqs. used for AM and panning
  kf linsegb 0.1, p3*0.62, 0.8, p3*0.8, 0.8, p3*0.96, 12, p3, 60
  kLfoFreqs[] = iFreqs * kf 
  
  ; multiple noise instances, amplitude modulated
  aA[]    poly inum, "noise", 0.5, 0.3
  aAmps[] poly inum, "oscili", 1, kLfoFreqs*0.6
  aA *= aAmps

  ; filter noise with bandpass 
  kQ linsegb 0.5, p3*0.5, 0.02, p3, 0.0001
  kBands[] = kFreqs * kQ
  aB[] poly inum, "resonr", aA, kFreqs, kBands
  
  ; panning. poly works also with k-rate and with 
  ; opcodes producing multiple outputs, like pan2
  kPanPos[] poly inum, "lfo", 0.5, kLfoFreqs
  kPanPos += 0.5  ; lfo in the range 0-1 for panning
  aL[], aR[] poly inum, "pan2", aB, kPanPos
  
  aleft  sumarray aL 
  aright sumarray aR

  ; compress / fade
  aref init 1 
  asig = 0.707 * (aleft + aright)
  again compress2 aref, asig, -90, -48, -24, 2.5, 0.05, 0.2, 0.05
  aleft *= again 
  aright *= again 

  again2 compress2 aref, (aleft+aright)*0.707, -90, -6, -3, 20, 0.002, 0.010, 0.02
  aenv = again2 * cossegr:a(0, 1, 1, 0.1, 0)
  outs aleft*aenv, aright*aenv
endin

opcode test, a, k
  kfreq xin 
  aout oscili 0.1, kfreq 
  xout aout 
endop 

instr 3
  ; test udo
  kfreqs[] fillarray 440, 443 
  aA[] poly 2, "test", kfreqs 
  a0 sumarray aA
  outs a0, a0 
endin 
      
</CsInstruments>
<CsScore>

i 1 0 8
; i 2 9 50
; i 3 0 1

</CsScore>
</CsoundSynthesizer>
