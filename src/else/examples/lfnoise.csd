<CsoundSynthesizer>
<CsOptions>
-odac           
   
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

FLpanel "lfnoise", 400, 200, 50, 50
	idisp1 FLvalue "", 50, 30, 322, 20
	idisp2 FLvalue "", 30, 30, 52, 80
	FLcolor 150, 100, 150, 200, 100, 250
	gkfreq,   ih1 FLslider "freq",   0, 200, 0, 3, idisp1, 300, 30, 20, 20
	gkinterp, ih2 FLslider "interp", 0, 1,   0, 3, idisp2,  30, 30, 20, 80
	gkgain,   ih3 FLslider "gain",   0, 1,   0, 3, -1,     300, 30, 20, 140
FLpanelEnd
FLrun

instr 1
	aout lfnoise gkfreq, gkinterp
    aout *= interp(gkgain)    
	outs aout, aout
endin

</CsInstruments>

<CsScore>
i1 0 100

</CsScore>
</CsoundSynthesizer>
