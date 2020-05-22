<CsoundSynthesizer>
<CsOptions>
-m0
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

gidict dict_new "*sf"

opcode argset, 0, iSk
  ip1, Sparam, kvalue xin
  Skey sprintf "%.4f:%s", ip1, Sparam
  dict_set gidict, Skey, kvalue
endop

opcode argget, k, Si
  Sparam, idefault xin
  Skey sprintf "%.4f:%s", p1, Sparam
  kout dict_get gidict, Skey, idefault
  xout kout
  ;; delete key at end of event
  defer "dict_set", gidict, Sparam 
endop

instr exit
  prints "Exiting csound \n"
  exitnow
endin

instr 1
  inum uniqinstance 2
  kfreq = linseg(random(4000, 2000), p3, random(300, 350))
  argset inum, "freq", kfreq
  schedule inum, 0, 3
  ksize dict_size gidict
  printk2 ksize
  atstop p1, 0, p3
endin

instr 2
  kfreq argget "freq", 1000 
  a0 oscili 0.02, kfreq
  a0 *= linsegr:a(0, 0.1, 1, 0.1, 0)
  outs a0, a0
endin

instr example_dict
  schedule 1, 0, 0.01
  schedule "exit", 10, 0.1
  turnoff
endin

;; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

instr 10
  inum uniqinstance 11, 10000
  kfreq = linseg(4000, p3, random(300, 350))
  Schan = sprintf("%f_freq", inum)
  dict_set gidict, Schan, kfreq
  schedule inum, 0, p3
endin

instr 11
  Skey sprintf "%f_freq", p1
  printf "p1=%.6f \n", 1,  p1
  kfreq dict_get gidict, Skey, 1000
  ;; delete key at end of event
  defer "dict_set", gidict, Skey
  a0 oscili 0.02, kfreq
  a0 *= linsegr:a(0, p3*0.1, 1, p3*0.9, 0)
  outs a0, a0
endin

instr dictsize
  isize = dict_size:i(gidict)
  print isize
  turnoff
endin

instr example2
  i0 = 0
  istep = 0.01
  idur = 0.3
  while i0 < 10000 do 
    schedule 10, i0 * istep, idur
    i0 += 1
    print i0
  od
  iendtime = (i0 + 2) * istep + idur
  schedule "dictsize", iendtime-0.005, -1
  schedule "exit", iendtime, -1
  turnoff
endin

;; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

instr 20
  inum uniqinstance 21, 10000
  kfreq = linseg(4000, p3, random(300, 350))
  Schan = sprintf("%f_freq", inum)
  ; printf "%s \n", 1, Schan
  chnset kfreq, Schan
  schedule inum, 0, p3
endin

instr 21
  Skey sprintf "%f_freq", p1
  printf "p1=%.6f \n", 1,  p1
  kfreq chnget Skey
  a0 oscili 0.02, kfreq
  a0 *= linsegr:a(0, p3*0.1, 1, p3*0.9, 0)
  outs a0, a0
endin

instr example3
  ; the same as example2 but with channels
  i0 = 0
  istep = 0.01
  idur = 0.3
  while i0 < 10000 do 
    schedule 20, i0 * istep, idur
    i0 += 1
    print i0

  od
  schedule "exit", (i0 + 1) * istep + idur, -1
  turnoff
endin

; schedule "example_dict", 0, 1
schedule "example2", 0, 1
; schedule "example3", 0, 1


</CsInstruments>
e 10
<CsScore>

</CsScore>
</CsoundSynthesizer>