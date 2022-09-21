<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

sr	= 44100
ksmps = 32
nchnls	= 2
0dbfs	= 1

instr nothing
endin

instr john
  prints "instrument name = %s\n", nstrstr(p1)
  prints "instrument number = %d\n", nametoinstrnum("john")
  prints "Non existing instrument: %d\n", nametoinstrnum("foo")
endin

instr test
endin

</CsInstruments>
<CsScore>
i "john" 0 0
</CsScore>
</CsoundSynthesizer>
