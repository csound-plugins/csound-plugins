<CsoundSynthesizer>
<CsOptions>

</CsOptions>

<CsInstruments>
sr     = 48000
ksmps  = 64
nchnls = 2
0dbfs  = 1


instr 1
	ain init 0
	afoo testargs ain
endin


</CsInstruments>

<CsScore>
i 1  0 1

</CsScore>
</CsoundSynthesizer>
