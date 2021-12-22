<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
  idict dict_new "sf", "foobadooh", 1, "bar", 2, "baz", 0.5
  Skey = "foobadooh"
  k0 = 0
  while (k0 < 1000) do
    kfoo dict_get idict, "foobadooh"
    k0 += 1
  od
  printf "foo= %f \n", accum(changed(kfoo)), kfoo  
endin

instr 2
  idict dict_new "int:str", 0, "foo", 1, "jiji", 10, "bar"
  dict_set idict, 1, "jiji2"
  prints "0: %s\n", dict_get:S(idict, 0)
  S1 dict_get idict, 1
  println "1: %s\n", S1
  dict_print idict 
  turnoff
    
  
endin

</CsInstruments>

<CsScore>

; i1 0 10
i2 0 0.1
; f0 3600

</CsScore>
</CsoundSynthesizer>
