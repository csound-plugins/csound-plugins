<CsoundSynthesizer>
<CsOptions>
; -odac           
; -iadc     
; -d     ;;;RT audio I/O
;; -+rtaudio=jack
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

opcode rndarr, i[], i
  ; create an array of inum size, fill it with random numbers betwen 0-1
  inum xin 
  iout[] init inum 
  ii = 0
  while ii < inum do 
    iout[ii] = rnd(1)
    ii += 1
  od 
  xout iout
endop

instr 1
  ; multiple oscillators, mixed down to mono

  ; number of oscillators
  inum = 100

  ; each oscillator is an overtone
  kratios[] genarray_i 1, inum
  
  ; randomize the phases to avoid synchronous start
  iphs[] rndarr inum 

  ; fundamental
  km0 line ntom:i("2G"), p3, ntom:i("1C")

  ; change the harmonicity over time
  kexp line 1, p3, 1.32

  ; calculate the freq. of each oscillator
  kfreqs[] = (kratios ^ kexp) * mtof(km0)

  ; generate the oscillators. 
  aa[] poly inum, "oscili", 1/inum, kfreqs, -1, iphs

  amono sumarray aa

  ; declick
  amono *= linsegr:a(0, 0.05, 1, 0.05, 0)
  outch 1, amono
endin

instr 2
  ; poly instances can be stacked as a processing pipe

  inum = 20
  kratios[] genarray_i 1, inum
  iphs[] rndarr inum 

  km0 line ntom:i("3G"), p3, ntom:i("2C")
  kp line 1, p3, 1.62
  kfreqs[] = (kratios ^ kp) * mtof(km0)

  aref noise 0.5, 0
  aa[] poly inum, "noise", 0.5, 0.3
  kbands[] = kfreqs * 0.005
  ab[] poly inum, "butterbp", aa, kfreqs, kbands
  a0 sumarray ab
  a0 balance2 a0, aref
  a0 *= linsegr:a(0, 0.1, 0.5, 0.1, 0)
  a0 = limit(a0, -1, 1)
  outch 1, a0 
endin

instr 4
  ; test filtering 2
  inum = 50
  kratios[] genarray_i 1, inum
  inuminst lenarray kratios
  iphs[] rndarr inum 

  km0 line ntom:i("2G"), p3, ntom:i("1C")
  kp line 1, p3, 1.32
  kfreqs[] = (kratios ^ kp) * mtof(km0)
  aa[] poly inum, "vco2", 1/inuminst, kfreqs, 0
  kcutoff lincos line(0, p3, 1), 5000, 400
  kq = kcutoff / 100
  aa[] poly inum, "lowpass2", aa, kcutoff, kq;; [, inlp, isaturation, istor]"
  ; aa[] poly inum, "K35_lpf", aa, kcutoff, 9.99, 1, 1
  a0 sumarray aa
  a0 compress2 a0, a0, -90, -20, -12, 2, 0.005, 0.1, 0.010
  a0 *= linsegr:a(0, 0.05, 1, 0.05, 0)
  outch 1, a0 
endin

instr 5
  kfreqs[] fillarray 230, 233, 440, 443, 450, 455, 1000, 1010
  inum lenarray kfreqs
  aa[] poly inum, "vco2", 0.2, kfreqs
  kcf = expon(8000, p3, 100)
  ;; aa   poly inum, "butlp", aa, kcf
  aa poly inum, "moogladder", aa, kcf, 0.9
  a0 sumarray aa
  a0 *= 0.8
  a0 *= linsegr:a(0, 0.05, 1, 0.05, 0)
  outch 1, a0 
endin

instr 6
  ; test k opcode
  ifreqs[] fillarray 440, 443, 500, 510, 800, 804
  inum = lenarray:i(ifreqs)
  kfactor linsegb 1, 1.999, 1, 2, 2
  kfreqs2[] = ifreqs * kfactor
  klag = 0.5
  kfreqs3[] poly inum, "sc_lag", kfreqs2, klag
  aouts[] poly inum, "oscili", 0.1, kfreqs3
  outch 1, sumarray(aouts)
endin 

instr 10
  ; test combinations of array and scalar inputs
  kfreqs[] fillarray 1000, 1100, 6000
  kbws[] fillarray 15, 15, 200
  a0 noise 0.2, 0
  ain[] init 3
  ain = a0 
  
  aa[] poly 3, "reson", a0, kfreqs, kbws
  aa[] poly 3, "balance", aa, ain
  aout sumarray aa
  outch 1, aout
endin
          
</CsInstruments>
<CsScore>

i 2 0 5
f0 6

</CsScore>
</CsoundSynthesizer>