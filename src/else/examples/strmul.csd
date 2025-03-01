<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

instr 1
	Sout = strmul("*", 10)
	println "Sout: %s", Sout
	turnoff
endin

instr 2
	ksig = oscil:k(0.5, 5)+0.5
	Ssig = strmul("*", round(ksig*80));
	printsk "ksig: %s\n", Ssig
endin
</CsInstruments>

<CsScore>

i 2 0 1
</CsScore>
</CsoundSynthesizer>
