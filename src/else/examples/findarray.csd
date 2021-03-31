<CsoundSynthesizer>
<CsOptions>
--nosound
; -odac
</CsOptions>

<CsInstruments>

/*

Description
===========

findarray return the index of the first element which is equal to the 
given number. If the number is not found, the return value is -1

Syntax
======

    kidx findarray karray[], kvalue/ivalue [, iepsilon=1e-12]
    kidx findarray iarray[], kvalue [, iepsilon=1e-12]
    idx  findarray iarray[], ivalue [, iepsilon=1e-12]
    
*/

ksmps = 64
nchnls = 2
0dbfs  = 1

instr 10
    karr[] fillarray 0, 0.5, 0.3, 10, 0.8
    kidx findarray karr, 0.3
    println "kidx: %d", kidx

    karr2[] genarray_i 0, 100, 0.1
    kidx2 findarray karr2, 70.8
    println "kidx2: %d", kidx2
    turnoff
endin

</CsInstruments>

<CsScore>

i 10 0 0.1

</CsScore>
</CsoundSynthesizer>
