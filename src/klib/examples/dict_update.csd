<CsoundSynthesizer>
<CsOptions>
-m0
--nosound
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
	idict1 dict_new "sf", "foo", 1, "bar", 2
	idict2 dict_new "sf", "bar", 20, "baz", 30
	dict_update idict1, idict2
	dict_print idict1
	turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1

</CsScore>
</CsoundSynthesizer>
