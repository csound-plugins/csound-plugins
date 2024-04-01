<CsoundSynthesizer>
<CsOptions>

-odac
-m0

</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

instr exit
  exitnow
endin
  
;; Example 1. instr 2 creates and controls instr 1
  
instr 1
  pset 0, 0, 0, 40, 50
  kt timeinsts
  k4 = p4
  k5 = p5
  printf "time: %.4f\tinstance: %.3f\tp4: %f\tp5: %f \n", metro(20), kt, p1, k4, k5
endin

instr 2
  kval line 0, p3, 1
  pwrite 1.01, 4, kval
  pwrite 1.02, 5, kval*2
endin

instr example1
  schedule 1.01, 0, 4, -1
  schedule 1.02, 0, 4, -1
  schedule 2,    1, 1
  schedule "exit", 4, -1
  turnoff
endin

;-----------------------------
; Example 2, one instrument modulates another

instr ex2_generator
  pset p1, p2, p3, 0.5, 1000, 4000, 0.1
  kamp       = p4
  kfreq      = p5
  kcutoff    = p6
  kresonance = p7
  asaw vco2, kamp, kfreq
  aout moogladder2, asaw, kcutoff, kresonance
  aout *= linsegr(0, 0.1, 1, 0.1, 0)
  outs aout, aout  
endin

instr ex2_control
  iglissdur = p4
  inum = nstrnum("ex2_generator")
  inum1 = inum + 0.001
  inum2 = inum + 0.002
  kfreq1 linseg ntof("4A"), iglissdur, ntof("3A")
  kfreq2 linseg ntof("4F"), iglissdur, ntof("3F")
  ;                      amp
  schedule inum1, 0, p3, 0.2 
  schedule inum2, 0, p3, 0.2
  pwrite inum1, 5, kfreq1
  pwrite inum2, 5, kfreq2
endin

instr ex2_broadcast
  printf "filter start\n", 1
  inum = nstrnum("ex2_generator")
  kcutoff    linseg 4000, p3, 400
  kresonance linseg 0.1, p3*0.5, 0.8
  pwrite inum, 6, kcutoff, 7, kresonance
endin

instr example2
  schedule "ex2_control", 0, 8, 4
  schedule "ex2_broadcast", 4, 4
  schedule "exit", 8.5, -1
  turnoff
endin

instr 100
  kfreq = p4
  outch 1, oscili:a(0.1, kfreq)
endin

instr pwriteonce
  ip1 = p4
  iidx = p5
  ivalue = p6
  pwrite ip1, iidx, ivalue
  turnoff
endin

instr example3
  ; pwrite in the future
  schedule 100.01, 2, 10, 2000
  schedule "pwriteonce", 0.5, 0, 100.01, 4, 440
  turnoff
endin

;; Uncomment as needed

; schedule "example1", 0, 1
; schedule "example2", 0, 1
schedule "example3", 0, 1

</CsInstruments>
<CsScore>

</CsScore>
</CsoundSynthesizer>
