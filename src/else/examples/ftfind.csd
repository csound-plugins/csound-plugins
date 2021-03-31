<CsoundSynthesizer>
<CsOptions>
--nosound
; -odac
</CsOptions>

<CsInstruments>

/*

Description
===========

ftfind return the index of the first element in a table which is equal 
to the given number. If the number is not found, the return value is -1

Syntax
======

    kidx ftfind ktabnum, kvalue, iepsilon=1e-12
    iidx ftfind itabnum, ivalue, iepsilon=1e-12
    
*/

ksmps = 64
nchnls = 2
0dbfs  = 1

instr 10
    itabnum ftfill 0, 0.5, 0.3, 10, 0.8
    iidx ftfind itabnum, 0.3
    prints "iidx: %d \n", iidx

    turnoff
endin

</CsInstruments>

<CsScore>

i 10 0 0.1

</CsScore>
</CsoundSynthesizer>
