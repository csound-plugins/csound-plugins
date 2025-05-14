<CsoundSynthesizer>
<CsOptions>
; -odac
</CsOptions>

<CsInstruments>

/*

Description
===========

ftnew creates an empty table of a given size


Syntax
======

    itabnum ftnew isize, idefault=0

**NB**: this is the same as ``itabnum ftgen 0, 0, isize, -2, idefault``

*/

ksmps = 64
nchnls = 2
0dbfs  = 1

instr example
    ; create a table from an array of any size
    ixs[] fillarray 1, 1, 2, 3, 5, 8, 13, 21, 34
    itab1 ftnew lenarray(ixs)
    copya2ftab ixs, itab1
    itab2 ftgen 0, 0, 0, 2, 1, 1, 2, 3, 5, 8, 13, 21, 34
    
    itab3 ftfill 1, 1, 2, 3, 5, 8, 13, 21, 34
    itab4 ftgen 0, 0, -9, 2, 0
    ftprint itab1
    ftprint itab2
    ftprint itab3
    ftprint itab4
    turnoff
endin

</CsInstruments>

<CsScore>

i "example" 0 1

</CsScore>
</CsoundSynthesizer>
