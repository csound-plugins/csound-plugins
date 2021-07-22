<CsoundSynthesizer>

<CsOptions>
-odac
</CsOptions>

<CsInstruments>

/*
    This is the example file for opcode "linenv"
    
    linenv is a triggerable envelope with a sustain segment

    aout linenv kgate, isustidx, kval0, ktime1, kval1, ..., ktimen, kvaln

    NB: use xtratim if necessary to allow for release segment 
    
*/


sr = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1
gkgate init 0

FLpanel "linenv", 240, 100, 100, 100
	gkgate, gih1 FLbutton " Gate", 1, 0, 2, 80, 40, 10, 10, -1  
FLpanelEnd
FLrun

instr 1
    kgate = trighold:k(metro(1/2), 0.5)
    kenv linenv kgate, 1, 0, 0.15, 1, 0.1, 0
    printf "t: %f,  kenv: %f \n", timeinstk(), timeinsts(), kenv

    kenv *= 0.2
    asig = pinker() * interp(kenv)
    outs asig, asig
endin

instr 2
    iperiod = 2
    igatedur = 1
    kgate = trighold:k(metro(1/iperiod), igatedur)
    aenv linenv kgate, -2, 0, 0.05, 1, 0.2, 0.5, 0.2, 1, 0.4, 0
    asig = oscili:a(0.2, 1000) * aenv
    FLsetVal changed(kgate), kgate, gih1
    outs asig, asig
endin

instr 3
	asig pinker
	aenv linenv gkgate, 2, 0, 0.05, 1, 0.1, 0.2, 0.5, 0
	asig *= aenv
	outs asig, asig
endin

instr 4
    ; no sustain ("one shot")
    asig pinker
	aenv linenv gkgate, 0, 0, 0.05, 1, 0.1, 0.2, 0.5, 0
	asig *= aenv
	outs asig, asig
endin
    
</CsInstruments>

<CsScore>

; i1 0 10
; i2 0 10
i3 0 100
; i4 0 100
</CsScore>
</CsoundSynthesizer>