<CsoundSynthesizer>

<CsOptions>
-odac
</CsOptions>

<CsInstruments>
sr = 48000
ksmps = 64
0dbfs = 1
nchnls = 2

; Ambisonics order
giOrder	=	1

instr 1	

iSetup	init 21 // binaural 2D

; array Ambisonics
iArraySize	=	(giOrder+1)^2
aAmbi[]	init	iArraySize

; output
iOutSize	init	nchnls
aOut[]	init	iOutSize

aAmbi[0],aAmbi[1],aAmbi[2],aAmbi[3]     diskin "AJH_eight-positions.amb"

aOut bformdec2 iSetup,aAmbi,0,1,400,0,"hrtf-48000-left.dat","hrtf-48000-right.dat" //order1

outs aOut[0], aOut[1]

endin 


</CsInstruments>


<CsScore>

i1	0	20

</CsScore>

</CsoundSynthesizer>

