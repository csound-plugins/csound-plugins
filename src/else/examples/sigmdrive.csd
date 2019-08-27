<CsoundSynthesizer>
<CsOptions>
-odac           
   
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

FLpanel "sigmdrive", 400, 200, 50, 50
	idisp1 FLvalue "", 50, 30, 322, 20
	idisp2 FLvalue "", 30, 30, 52, 80
	FLcolor 150, 100, 150, 200, 100, 250
	gkdrive, idrivehandle1 FLslider "drive", 0, 10, 0, 3, idisp1, 300, 30, 20, 20
	gkmode,  idrivehandle2 FLslider "mode",  0, 1, 0,  3, idisp2, 30, 30, 20, 80
FLpanelEnd
FLrun

instr 1
	ain oscili 0.2, 440
	aout sigmdrive ain, port:k(gkdrive, 0.2), gkmode
	outs aout, aout
endin

</CsInstruments>

<CsScore>

i1 0 100

</CsScore>
</CsoundSynthesizer>
