<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

instr 1
  kx linseg 0, p3, 1
  printf "kx=%f \n", accum(changed(kx)), kx
  ; the same without accum would only print the first time,
  ; since changed would return always 1 but printf expects an ever
  ; increasing trigger
endin

</CsInstruments>
<CsScore>

i1 0 0.1

</CsScore>
</CsoundSynthesizer>