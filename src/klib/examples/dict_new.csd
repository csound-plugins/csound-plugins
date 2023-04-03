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

instr example2
  idict dict_new "str:float"
  dict_set idict, "foo", 0.5
  dict_set idict, "bar", 1
  ifoo dict_get idict, "foo"
  ibar dict_get idict, "bar"
  prints "ifoo: %f, ibar: %f\n", ifoo, ibar
endin

; schedule "example1", 0, 1
schedule "example2", 0, 1

</CsInstruments>

<CsScore>

</CsScore>
</CsoundSynthesizer>
