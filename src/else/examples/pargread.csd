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
    print p4
endin

instr 2
    ip4 pargread 1.01, 4
    printf "<<<< p4 for instr 1.01 is %f >>>>\n", 1, ip4
    turnoff
endin

</CsInstruments>

<CsScore>
i 1.01 0 2 95
i 2 1 0.1

</CsScore>
</CsoundSynthesizer>
