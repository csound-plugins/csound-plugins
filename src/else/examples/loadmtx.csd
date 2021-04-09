<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
    iA[] loadmtx "test-float64.npy"
    printarray iA

    iB[] loadmtx "test-2D.npy"
    printarray iB

    iC[] loadmtx "test-int.npy"
    printarray iC
    
    turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1
; f0 3600

</CsScore>
</CsoundSynthesizer>
