<CsoundSynthesizer>
<CsOptions>
--nosound
-m0
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

gidict dict_new "sf"

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
  dict_del gidict, Sparam 
endop

instr exit
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

instr 10
  inum uniqinstance 11, 10000
  kfreq = linseg(random(4000, 2000), p3, random(300, 350))
  dict_set gidict, sprintf("%f:freq", inum), kfreq
  schedule inum, 0, 3
  ksize dict_size gidict
  ; printk2 ksize
  print inum
  atstop p1, 0, p3
endin

instr 11
  Skey sprintf "%f:freq", p1
  kfreq dict_get gidict, Skey, 1000
  dict_del gidict, Skey
  a0 oscili 0.02, kfreq
  a0 *= linsegr:a(0, 0.1, 1, 0.1, 0)
  outs a0, a0
endin


instr example2
  schedule 10, 0, 0.01
  schedule "exit", 50, 0.1
  turnoff
endin

instr 20
  inum uniqinstance 21
  kfreq = linseg(random(4000, 2000), p3, random(300, 350))
  Schan = sprintf("%f:freq", inum)
  chnset kfreq, Schan
  schedule inum, 0, 3
  atstop p1, 0, p3
endin

instr 21
  Skey sprintf "%f:freq", p1
  kfreq chnget Skey
  a0 oscili 0.02, kfreq
  a0 *= linsegr:a(0, 0.1, 1, 0.1, 0)
  outs a0, a0
endin

instr example3
  ; the same as example2 but with channels
  schedule 20, 0, 0.01
  schedule "exit", 50, 0.1
  turnoff
endin

; schedule "example_dict", 0, 1
schedule "example2", 0, 1
; schedule "example3", 0, 1


</CsInstruments>

<CsScore>

</CsScore>
</CsoundSynthesizer>