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
    iArr[] fillarray 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ; set even indexes to -1
    setslice iArr, -1, 0, 0, 2
    printarray iArr

    ; copy iA to iDest starting at index 10
    iDest[] init 20
    iA[] genarray 100, 109
    setslice iDest, iA, 10
    printarray iDest, "%d "
    turnoff
endin



</CsInstruments>

<CsScore>
i1 0 0.1

; f0 3600

</CsScore>
</CsoundSynthesizer>
