<CsoundSynthesizer>

<CsOptions>
-odac
</CsOptions>

<CsInstruments>
/*
    This is the example file of rampgate
    rampgate is a triggerable envelope with a sustain segment

    aout rampgate kgate, isustidx, kval0, ktime1, kval1, ..., ktimen, kvaln

    NB: use isustidx=-1 if no sustain is desired
    NB: use xtratim if necessary to allow for release segment 
    
*/


sr = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1
gkgate init 0

FLpanel "rampgate", 240, 100, 100, 100
	gkgate, ih1 FLbutton " Gate", 1, 0, 2, 80, 40, 10, 10, -1  
FLpanelEnd
FLrun

instr 1
    kgate = sc_trig:k(metro(1/2), 0.5)
    kenv rampgate kgate, 1, 0, 0.15, 1, 0.1, 0
    printf "t: %f,  kenv: %f \n", timeinstk(), timeinsts(), kenv

    kenv *= 0.2
    asig = pinker() * interp(kenv)
    outs asig, asig
endin

instr 2
    iperiod = 2
    igatedur = 1
    kgate = sc_trig:k(metro(1/iperiod), igatedur)
    aenv rampgate kgate, 1, 0, 0.2, 1, 0.1, 0
    asig = oscili:a(0.2, 1000) * aenv
    outs asig, asig
endin

instr 3
	asig pinker
	aenv rampgate gkgate, 2, 0, 0.05, 1, 0.1, 0.2, 0.5, 0
	asig *= aenv
	outs asig, asig
endin

</CsInstruments>

<CsScore>

; i1 0 10
; i2 0 10
i3 0 100

</CsScore>
</CsoundSynthesizer>