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
    pset 0, 0, 0, 40, 50, 60
    k4 = p4
    k5 = p5
    printf "instance: %.3f, p4: %f, p5: %f \n", metro(20), p1, k4, k5
endin

instr 2
    kval line 10, p3, 20
    pargwrite 1.01, 4, kval
    pargwrite 1.02, 5, kval*2
endin

</CsInstruments>

<CsScore>
i 1.01 0 2 101
i 1.02 0 2 102
i 2 1 0.5

</CsScore>
</CsoundSynthesizer>
