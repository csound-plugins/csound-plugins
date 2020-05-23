<CsoundSynthesizer>
<CsOptions>

</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1

opcode csgain, aa, aakk
setksmps 1
    a1, a2, kgain1, kgain2 xin
    a1 *= kgain1
    a2 *= kgain2
    xout a1, a2
endop

instr 1
	a1 oscili 1, 1000
	a2 pinker
	kslider2 line 1, p3, 0
	kslider1 line 0, p3, 1
	ih, a1, a2 jsfx "gain.jsfx", a1, a2, 1, kslider1, 2, kslider2
    outch 1, a1, 2, a2
endin

instr 2
    a1 oscili 1, 1000
	a2 pinker
	kslider2 line 1, p3, 0
	kslider1 line 0, p3, 1
	a1 = a1 * kslider1
	a2 = a2 * kslider2
	outch 1, a1, 2, a2
endin
    
instr 3
    a1 oscili 1, 1000
	a2 pinker
	kslider2 line 1, p3, 0
	kslider1 line 0, p3, 1
    a1, a2 csgain a1, a2, kslider1, kslider2
	outch 1, a1, 2, a2
endin

</CsInstruments>

<CsScore>

i1 0 10
; i2 0 10
; i3 0 10
</CsScore>
</CsoundSynthesizer>