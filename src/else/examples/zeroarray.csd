<CsoundSynthesizer>
<CsOptions>
; -odac
</CsOptions>

<CsInstruments>

/*

Description
===========

zeroarr zeroes all elements in an array of any (numeric) kind

Syntax
======

    zeroarr karr
    zeroarr arr

*/

ksmps = 32
nchnls = 2
0dbfs  = 1

gabuses[] init 4

instr 10
    asig vco2 0.1, 1000
    gabuses[0] = gabuses[0] + asig
endin

instr 20
    asig = gabuses[0]
    outch 1, asig
    zeroarray gabuses
endin


</CsInstruments>

<CsScore>

i 10 0 10
i 20 0 10

</CsScore>
</CsoundSynthesizer>
