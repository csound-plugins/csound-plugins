<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

ksmps = 64
nchnls = 2

gk_dict init 0

instr 1
  idict dict_new "sf"  ; local
  gk_dict = idict
  kt timeinstk
  dict_set idict, "foo", kt
  dict_set idict, "bar", kt*2
endin

instr 2
  idict = i(gk_dict)
  print idict
  ktim timeinsts
  kt timeinstk
  ktype dict_type idict
  printf "ktim: %f   exists: %d \n", kt, ktim, ktype > 0 ? 1 : 0
  
endin

</CsInstruments>
<CsScore>
i 1 0.5 1
i 2 0 1

</CsScore>
</CsoundSynthesizer>