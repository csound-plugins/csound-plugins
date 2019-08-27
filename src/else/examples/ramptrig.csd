<CsoundSynthesizer>
<CsOptions>

</CsOptions>

<CsInstruments>
sr = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

; Use Case #1: An envelope which can be retriggered
instr 1
    ; Duration of envelope
    kdur = 1
    ; This is the gate, could be any irregular signal, midi, osc, etc.    
    ktrig metro 0.5
    ; Whenever ktrig is possitive and higher than previous value, 
    ; kx ramps from 0 to 1 in kdur seconds
    kx ramptrig ktrig, kdur
    ; actual envelope
    kenv bpf kx*kdur, 0, 0, 0.02, 1, kdur, 0
    asig oscili 0.2, 1000
    ; asig pinker
    asig *= interp(kenv)
    outs asig, asig
endin

; Use Case #2: Use finished trigger to signal something
instr 2
    ktrig metro 1/4
    ktrig delayk ktrig, 0.5
    idur = 2
    kphase, kfinished1 ramptrig ktrig, 2
    printf "finished! \n", kfinished1
    kenv bpf kphase * idur, 0, 0, 0.5, 1, 0.8, 0.5, 1, 1, idur, 0
    asig = pinker() * interp(kenv)
    outs asig, asig
endin


</CsInstruments>

<CsScore>

; i1 0 10
i2 0 12
; i3 0 20
; f0 3600


</CsScore>
</CsoundSynthesizer>