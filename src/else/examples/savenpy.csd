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
	itab ftfill 1, 10, 20, 20, 3, 30, 0.5, 5
	savenpy "test.npy", itab
	turnoff
endin

</CsInstruments>

<CsScore>

i1 0 0.1
; f0 3600

</CsScore>
</CsoundSynthesizer>
