<CsoundSynthesizer>
<CsOptions>
-odac           
   
</CsOptions>

<CsInstruments>

/*
    Example file for lfnoise

    lfnoise generates a random value between 0-1 at the given
    frequency. If kinterp=1, then values are interpolated; otherwise,
    they are held until next value

*/

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

FLpanel "lfnoise", 400, 200, 50, 50
	idisp1 FLvalue "", 50, 30, 322, 20
	FLcolor 150, 100, 150, 200, 100, 250
	gkfreq,   ih1 FLslider "freq",   0, 200, 0, 3, idisp1, 300, 30, 20, 20
	gkinterp, ih2 FLbutton "interpolate", 1, 0,   3, 100, 50, 20, 80, -1
	gkgain,   ih3 FLslider "gain",   0, 1,   0, 3, -1,     300, 30, 20, 140
FLpanelEnd
FLrun
FLsetVal_i 8, ih1
FLsetVal_i 0.1, ih3

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
