<CsoundSynthesizer>
<CsOptions>

--nosound
-m0

</CsOptions>

<CsInstruments>

/*
    Example file for pargread

    ivalue pargread instrnum, indx [, inotfound=-1]

    pargread reads a parg value from an active instrument
    Returns inotfound if instrnum is not active
   
*/

instr 1
    k4 = p4
    printf "instance: %.3f, p4: %f \n", metro(10), p1, k4
endin

instr 2
    kval line 10, p3, 20
    pargwrite 1.01, 4, kval
endin

</CsInstruments>

<CsScore>
i 1.01 0 2 101
i 1.02 0 2 102
i 2 1 0.5

</CsScore>
</CsoundSynthesizer>
