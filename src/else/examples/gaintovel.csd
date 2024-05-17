<CsoundSynthesizer>
<CsOptions>
--nosound

</CsOptions>

<CsInstruments>

0dbfs = 1

instr 1
  prints ">> Instr 1\n"
  imindb = -60
  idb = -40
  while idb <= 0 do
  	igain = db(idb)
    ivel = gaintovel(igain, db(imindb), 1/3)
    prints ">> Gain: %d dB, \tamplitude: %f, \tvelocity: %f\n", idb, igain, ivel
    idb += 2
  od
  turnoff
endin


</CsInstruments>
<CsScore>

i1 0 1

</CsScore>
</CsoundSynthesizer>
