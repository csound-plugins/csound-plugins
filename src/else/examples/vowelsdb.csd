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
	ifreqs[][], ibws[][], iamps[][] vowelsdb "csound-tenor", "a e i o   u"
	printarray ifreqs, "%d", "ifreqs ="
	printarray ibws, "%d", "ibws ="
	printarray iamps, "", "iamps ="
endin

instr 2
	kfreqs[][], kbws[][], kamps[][] vowelsdb "csound-bass", "a e i o   u"
	println "kfreqs ="
	printarray kfreqs
	println "kbws ="
	printarray kbws
	println "kamps ="
	printarray kamps
	turnoff
endin

</CsInstruments>

<CsScore>

i1 0 0.1
i2 0 0.1
; f0 3600

</CsScore>
</CsoundSynthesizer>
