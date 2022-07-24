<CsoundSynthesizer>
<CsOptions>
-odac           
   
</CsOptions>

<CsInstruments>

/* 

    sigmdrive: a sigmoid distortion

    aout sigmdrive ain, kdrive, kmode=0

    kdrive: how much distortion (range 0-1)
    kmode: 0 = tanh, 1 = pow
    
*/

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

FLpanel "sigmdrive", 400, 200, 50, 50
	idisp1 FLvalue "", 50, 30, 322, 20
	FLcolor 150, 100, 150, 200, 100, 250
	gkdrive, idrivehandle1 FLslider "drive", 0, 10, 0, 3, idisp1, 300, 30, 20, 20
	gkmode, idrivehandle2  FLbutton "mode", 1, 0, 3, 60, 50, 20, 80, -1
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
