<CsoundSynthesizer>
<CsOptions>

--nosound

</CsOptions>

<CsInstruments>

instr example1
  idict dict_new "int:float", 32
  i0 = 0
  while i0 < 20 do
    ival = i0*2
    dict_set idict, i0, ival
    i0 += 1
  od
  dict_print idict
  exitnow
endin

schedule "example1", 0, 1


</CsInstruments>

<CsScore>

</CsScore>
</CsoundSynthesizer>