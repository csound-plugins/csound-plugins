<CsoundSynthesizer>
<CsOptions>
-odac           
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

/* Example file for standardchaos opcode

	aout standardchaos krate, kk, ix=0.5, iy=0
	
	Standard map chaotic generator, the sound is generated with the difference equations;
    y[n] = (y[n-1] + k * sin(x[n-1])) % 2pi;
    x[n] = (x[n-1] + y[n]) % 2pi;
    out = (x[n] - pi) / pi;

*/

FLpanel "standardchaos", 600, 300, 50, 50
	idisp1 FLvalue "", 50, 30, 522, 20
	idisp2 FLvalue "", 50, 30, 522, 80
	FLcolor 150, 100, 150, 200, 100, 250
	gkrate,   gih1 FLslider "rate", 0, 20000, 0, 3, idisp1, 500, 30, 20, 20
	gkk,      gih2 FLslider "k",    0, 10,    0, 3, idisp2, 500, 30, 20, 80
FLpanelEnd
FLrun

FLsetVal_i 1618, gih1
FLsetVal_i 1,    gih2

instr 1
	ix = 0.5
	iy = 1
	igain = 0.3
	
	aout standardchaos gkrate, gkk, ix, iy
	aout *= igain
	outs aout, aout	
endin

</CsInstruments>

<CsScore>
i1 0 100

</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>100</x>
 <y>100</y>
 <width>320</width>
 <height>240</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
