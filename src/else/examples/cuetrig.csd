<CsoundSynthesizer>
<CsOptions>

</CsOptions>

<CsInstruments>

0dbfs = 1

instr 1
  kt = eventtime() % 2
  ktrig cuetrig kt, 0.0, 0.1, 0.12, 0.4
  knotes[] fillarray 60, 64, 67, 67.5
  if ktrig != 0 then 
    println "ktrig: %d, kt: %f, note: %f", ktrig, kt, knotes[ktrig-1]
    schedulek 2, 0, 0.5, knotes[ktrig-1]
  endif
endin

instr 2
  ifreq = mtof:i(p4)
  outch 1, vco2:a(0.1, ifreq) * linsegr:a(0, 0.01, 1, 0.05, 0.1, 0.2, 0)
endin

</CsInstruments>
<CsScore>

i1 0 8

</CsScore>
</CsoundSynthesizer>
