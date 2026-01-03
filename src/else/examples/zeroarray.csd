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
nchnls = 8
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

instr 30
	; test masked zeroying
	kfreqs[] fillarray 200, 300, 400, 500, 600, 700, 800, 900
	asigs[] init 8
	asigs[0] = oscili:a(0.1, 200)
	asigs[1] = oscili:a(0.1, 300)
	asigs[2] = oscili:a(0.1, 400)
	asigs[3] = oscili:a(0.1, 500)
	asigs[4] = oscili:a(0.1, 600)
	asigs[5] = oscili:a(0.1, 700)
	asigs[6] = oscili:a(0.1, 800)
	asigs[7] = oscili:a(0.1, 900)
		
	; imask[] fillarray 0, 0, 1, 0, 1, 0, 0, 0
	imask ftfill 0, 0, 1, 0, 1, 0, 0, 0
	zeroarray asigs, imask
	out asigs
endin

</CsInstruments>

<CsScore>

; i 10 0 10
; i 20 0 10
i 30 0 10
</CsScore>
</CsoundSynthesizer>
