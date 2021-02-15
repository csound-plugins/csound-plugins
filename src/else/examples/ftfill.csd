<CsoundSynthesizer>
<CsOptions>
; -odac
</CsOptions>

<CsInstruments>

/*

Description
===========

ftfill creates a table and fills it with values. Like fillarray, but for f-tables.


Syntax
======

    itabnum ftfill x0, [x1, x2, ...]

**NB**: this is the same as ``itabnum ftgen 0, 0, 0, -2, x0, x1, x2, ...``

*/

ksmps = 32
nchnls = 2
0dbfs  = 1

instr example1
    itime2midi1 ftfill 0, 64, 4, 62, 5, 62, 6, 67
    itime2midi2 ftfill 0, 60, 4, 60, 5, 59, 6, 59
    ftprint itime2midi1
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

</CsInstruments>

<CsScore>

i "example1" 0 1

</CsScore>
</CsoundSynthesizer>
