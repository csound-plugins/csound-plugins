<CsoundSynthesizer>
<CsOptions>
-odac 
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

gifunc0 ftgen 0, 0, 0, -1, "bourre-fragment-1.flac", 0, 0, 0

gi1 ftgen 0, 0, nsamp(gifunc0), 2, 0
ftslicei gifunc0, gi1, 0, 0, 2
ftsetparams gi1, ftsr:i(gifunc0), 1

gi2 ftgen 0, 0, nsamp(gifunc0), 2, 0
ftslicei gifunc0, gi2, 1, 0, 2
ftsetparams gi2, ftsr:i(gifunc0), 1
    
instr 1
	a0, a1 loscil 1, 1, gifunc0, 1
	outch 1, a0, 2, a1
endin

instr 2
    a0 loscil 1, 1, gi1, 1
	a1 loscil 1, 1, gi2, 1
    outch 1, a0, 2, a1
endin

; schedule(1, 0, -1)
schedule(2, 0, -1)
</CsInstruments>
<CsScore>

</CsScore>
</CsoundSynthesizer>

