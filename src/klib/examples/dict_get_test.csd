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
  idict dict_new "*sf", "foobadooh", 1, "bar", 2, "baz", 0.5
  Skey = "foobadooh"
  k0 = 0
  while (k0 < 1000) do
    kfoo dict_get idict, "foobadooh"
    k0 += 1
  od
  printf "foo= %f \n", accum(changed(kfoo)), kfoo  
endin

</CsInstruments>

<CsScore>

i1 0 10
; f0 3600

</CsScore>
</CsoundSynthesizer>