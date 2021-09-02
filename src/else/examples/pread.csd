<CsoundSynthesizer>
<CsOptions>

--nosound
-m0

</CsOptions>

<CsInstruments>

/*

  Example file for pread
  ======================

  ivalue pread instrnum, index  [, inotfound=-1]
  kvalue pread instrnum, kindex [, inotfound=-1]

  pread reads a pfield value from an active instrument
  Returns inotfound if instrnum is not active

  Raises a performance error if index is out of range

*/

instr 1
  prints "instr 1. p4=%f, p5=%f\n", p4, p5
endin

instr 2
  ip1 = p4
  ip4 pread ip1, 4
  prints "Inside instr 2. Instance p1=%f, p4=%f\n", ip1, ip4
  pwrite ip1, 4, ip4*2
  turnoff
endin


instr 4
  ip1 = p4
  iindex[] fillarray 4, 5
  ivals[] pread ip1, iindex
  prints "Inside instr 4, reading p4 and p5 as array"
  printarray ivals
  turnoff 
endin

</CsInstruments>

<CsScore>
i 1.01 0   2 44   45
i 2    1   0 1.01
i 4    1.5 0 1.01

</CsScore>
</CsoundSynthesizer>
