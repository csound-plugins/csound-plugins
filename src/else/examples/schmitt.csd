<CsoundSynthesizer>
<CsOptions>
-odac           
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

/* Example file for schmitt opcode

	aout schmitt ain, khigh, klow
	kout schmitt kin, khigh, klow
	
	schmitt is a schmitt trigger (a gate with hysteresis), out is 1 if higher than khigh,
	0 if lower than klow

*/

FLpanel "schmitt", 400, 300, 50, 50
	idisp1 FLvalue "", 40, 30, 322, 20
	idisp2 FLvalue "", 40, 30, 322, 80
	idisp3 FLvalue "", 40, 30, 322, 140	
	FLcolor 150, 100, 150, 200, 100, 250
	gksignal, gih1 FLslider "signal", -1, 1, 0, 1, idisp1, 300, 30, 20, 20
	gklow,    gih2 FLslider "low",    -1, 1, 0, 3, idisp2, 300, 30, 20, 80
	gkhigh,   gih3 FLslider "high",   -1, 1, 0, 3, idisp3, 300, 30, 20, 140
	kschmitt, gih4 FLbutton "out",    1, 0, 3, 50, 50, 20, 200, -1
FLpanelEnd
FLrun

FLsetVal_i -0.5, gih2
FLsetVal_i 0.5, gih3

instr 1
	ain oscili 1, 0.25
	aout schmitt ain, gkhigh, gklow
	kguitrig metro 24
	FLsetVal kguitrig, k(ain), gih1
	FLsetVal kguitrig, k(aout), gih4	
endin

</CsInstruments>

<CsScore>
i1 0 100

</CsScore>
</CsoundSynthesizer>
