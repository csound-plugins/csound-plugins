<CsoundSynthesizer>
<CsOptions>

</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
	a1 oscili 1, 1000
	a2 pinker
	kslider2 line 1, p3, 0
	kslider1 line 0, p3, 1
	ih, a1, a2 jsfx "gain.jsfx", a1, a2, 1, kslider1, 2, kslider2
	print ih
    outch 1, a1, 2, a2
    jsfx_dump ih, metro(4)
endin


</CsInstruments>

<CsScore>

i1 0 4
</CsScore>
</CsoundSynthesizer>