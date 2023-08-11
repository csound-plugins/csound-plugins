<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>

<CsInstruments>

/*

Abstract
========

Determine the fractional index of a value within a sorted array / tab

Description
===========

Given an array x0, x1, x2, x3,..., determine the fractional index of a
value x. For example, given an array [0, 10, 14, 20], the value 12 would
receive the index 1.5 since it is to be placed between elements 1 and 2,
equidistant from both. The fractional part determines the relative distance
to the neighbouring elements.

Syntax
======

    kidx   bisect  kval, xarr[]
    iidx   bisect  ival, xarr[]
    aidx   bisect  aval, xarr[]
    kidx[] bisect  kvals[], xarr[]
    iidx[] bisect  ivals[], xarr[]

    kidx   bisect  kval, ktab, kstep=1, koffset=0
    iidx   bisect  ival, itab, istep=1, ioffset=0
    aidx   bisect  aval, ktab, kstep=1, koffset=0
    kidx[] bisect  kval[], ktab, kstep=1, koffset=0
    iidx[] bisect  ival[], itab, istep=1, ioffset=0

Args
====

    kval: the value to quiery within arr  
    arr: the array (1D) holding the data. 
    tab: the table holding the data
    step: in the case of a table, it is possible to bisect one particular
        column in the table if, for each row, multiple features are included
        in the same table
    offset: the offset determines the precise column to bisect
        
Output
======

    out: the index of val inside the array/tab  

See Also
========

interp1d, bpf, linlin, getrowlin, linenv
*/

ksmps = 128
nchnls = 2
0dbfs  = 1

instr example1
    ; used together with bisect can create multiple piecewise interpolation
    ; configurations
    itimes[] fillarray 0,   4,   5,    10
    imidi1[] fillarray 64, 64,   63.5, 64.5
    imidi2[] fillarray 64, 63.4, 63.4, 63
    iamps[]  fillarray 0,   0.8,   0.8,    0

    kidx bisect timeinsts(), itimes
    kamp interp1d kidx, iamps, "cos"
    aamp interp kamp
    a1 oscili aamp, mtof(interp1d(kidx, imidi1, "cubic"))
    a2 oscili aamp, mtof(interp1d(kidx, imidi2))
    println "amp: %f", rms:k(aamp)
    outch 1, a1, 2, a2
endin

instr example2
    ; a table can also be used with interp1d / bisect. A table can hold
    ; both x and y coordinates as pairs
    itime2midi1 ftfill 0, 64, 4, 62, 5, 62, 6, 67
    itime2midi2 ftfill 0, 60, 4, 60, 5, 59, 6, 59
    ftfree itime2midi1, 1
    ftfree itime2midi2, 1
    ; step=2, bisect the column 0.
    kt = timeinsts() 
    kidx1 bisect kt, itime2midi1, 2
    kidx2 bisect kt, itime2midi2, 2
        
    ; -1: cosine interpolation, step size=2, offset=1
    kmidi1 interp1d kidx1, itime2midi1, "cos", 2, 1
    kmidi2 interp1d kidx2, itime2midi2, "cos", 2, 1
        
    a0 squinewave a(mtof(kmidi1)), a(0.1), a(0.1)
    a1 squinewave a(mtof(kmidi2)), a(0.2), a(0.5)
    igain = 0.1
    ifade = 0.2
    aenv = linseg:a(0, ifade, igain, p3-ifade*2-0.1, igain, ifade, 0)
    outch 1, a0*aenv, 2, a1*aenv
endin

instr test1
    icurve ftfill 1, 60, 3, 65, 4, 60
    kt timeinsts
    kidx bisect kt, icurve, 2
    kmidi interp1d kidx, k(icurve), "linear", 2, 1
    println "kt: %f, kidx: %f, kmidi: %f", kt, kidx, kmidi
endin

instr test2
    ixs[] fillarray 0, 1, 2, 3
    iys[] fillarray 0, 10, 20, 30
    kt eventtime

    kidx bisect kt, ixs
    ky interp1d kidx, iys
    println "kt: %f, kidx: %f, ky: %f", kt, kidx, ky
endin

</CsInstruments>

<CsScore>

; Uncomment to perform each example

;i "example1" 0 10
;i "example2"  0 7
;i "test1" 0 5
i "test2" 0 4


</CsScore>
</CsoundSynthesizer>
