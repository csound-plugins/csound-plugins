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
  print p4
endin

instr 2
  ip4 pread 1.01, 4
  printf "<<<< instr 1.01 p4=%f >>>>\n", 1, ip4
  turnoff
endin

</CsInstruments>

<CsScore>
i 1.01 0 2   95
i 2    1 0.1

</CsScore>
</CsoundSynthesizer>
