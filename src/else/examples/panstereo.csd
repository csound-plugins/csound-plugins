<CsoundSynthesizer>
<CsOptions>
-odac              
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

opcode panst, aa, aak
  a0, a1, kpos xin
    aL,  aR  pan2 a0, kpos
    aL1, aR1 pan2 a1, kpos
    aL += aL1
    aR += aR1
    xout aL, aR
endop

instr 1
  anoise = pinker() * 0.5
  aL = reson(anoise, 2000, 200, 2)
  aR = reson(anoise, 1000, 200, 2)
  ; aR oscili 0.1, 1000
  kpan = lfo:k(0.5, 0.1) + 0.5
  println "kpan: %.3f", kpan
  aL, aR panstereo aL, aR, kpan, 1
  outs aL, aR
endin

instr 2
  anoise = pinker() * 0.5
  aL = reson(anoise, 2000, 10, 2)
  aR = reson(anoise, 400, 10, 2)
  ; aR oscili 0.1, 1000
  kpan invalue "pan"
  aL, aR panstereo aL, aR, kpan, 1
  outs aL, aR
endin

</CsInstruments>

<CsScore>

i2 0.1 100

</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>0</x>
 <y>0</y>
 <width>0</width>
 <height>0</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
 <bsbObject type="BSBKnob" version="2">
  <objectName>pan</objectName>
  <x>32</x>
  <y>10</y>
  <width>200</width>
  <height>200</height>
  <uuid>{96c2fd58-8a8e-4c1c-9be9-d33e4934e239}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.55500000</value>
  <mode>lin</mode>
  <mouseControl act="">continuous</mouseControl>
  <resolution>0.01000000</resolution>
  <randomizable group="0">false</randomizable>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <textcolor>#512900</textcolor>
  <border>0</border>
  <borderColor>#512900</borderColor>
  <showvalue>true</showvalue>
  <flatstyle>true</flatstyle>
  <integerMode>false</integerMode>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
