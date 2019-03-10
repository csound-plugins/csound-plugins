<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

ksmps = 64
nchnls = 2

gidict dict_new "sf"

instr 1
  idict dict_new "sf", 1
  dict_set gidict, "foo", idict
  dict_set idict, "bar", 10
  turnoff
endin

instr 2
  idict dict_get gidict, "foo"
  ibar dict_get idict, "bar"
  print idict
  print ibar
  turnoff
endin

instr 3
  idict dict_get gidict, "foo"
  dict_del idict, "bar"
  ibar dict_get idict, "bar"
  print ibar
  kbar dict_get idict, "bar", 999
  printk2 kbar
  turnoff
endin  


</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0.5 0.1
i 3 1 0.1

</CsScore>
</CsoundSynthesizer>