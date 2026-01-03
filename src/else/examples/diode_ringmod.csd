<CsoundSynthesizer>

<CsInstruments>

/*

    This is the example file for diode_ringmod

    To be run inside CsoundQt

    NB: diode_ringmod is a port of the jsfx plugin
    Loser/ringmodulator, which implements diode rectification
    and non linear behavior in the feedback path. 

    aout diode_ringmode a1, kmodfreq, kdiode=1, kfeedback=0, knonlin=0.2, koversample=0
        
    kmodfreq: frequency of the mod. signal
    kdiode: if 1, a diode rectification stage is applied to the mod. signal
    kfeedback: range is 0 to 1.
    knonlin: range 0 to 1, implements non-linearities in feedback and mod. freq (for 
        the first case only, which used the builtin oscillator)
    koversample: if 1, 2x oversampling is used.
    
*/

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

chn_k "modmidi", "r"
chn_k "feedback", "r"
chn_k "nonlinear", "r"
chn_k "oversample", "r"
chn_k "diode", "r"

gaOuts[] init 2


massign 1, 1
instr 1
  imidinote notnum
  ivel1 ampmidi 127
  ifreq mtof imidinote
  idb bpf ivel1, 0, -120, 64, -20, 90, -12, 127, 0
  iamp = ampdb(idb) * 0.2
  asig vco2 iamp, ifreq
  ; asig oscili iamp, ifreq
  aenv adsr 0.01, 0.1, 0.8, 0.2
  asig *= aenv
  gaOuts[0] = gaOuts[0] + asig

endin

instr 100
  kmodmidi = int(chnget:k("modmidi") * 4)/4
  if changed(kmodmidi) == 1 then
    outvalue "note", mton(kmodmidi)
  endif
  kdiode chnget "diode"
  kfeedback chnget "feedback"
  knonlinear chnget "nonlinear"
  koversample chnget "oversample"
  kmodfreq = mtof:k(kmodmidi)
  a1 = gaOuts[0]
  a2 diode_ringmod a1, kmodfreq, kdiode, kfeedback, knonlinear, koversample
  outs a2, a2
  gaOuts[0] = 0
endin

</CsInstruments>

<CsScore>
i 100 0 3600
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
 <bsbObject type="BSBKnob" version="2">
  <objectName>modmidi</objectName>
  <x>19</x>
  <y>19</y>
  <width>100</width>
  <height>100</height>
  <uuid>{4c2b5117-afe9-47b7-b0e7-ee5d8cc56120}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <minimum>24.00000000</minimum>
  <maximum>100.00000000</maximum>
  <value>66.14960000</value>
  <mode>lin</mode>
  <mouseControl act="">continuous</mouseControl>
  <resolution>0.01000000</resolution>
  <randomizable group="0">false</randomizable>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <textcolor>#f37b00</textcolor>
  <border>1</border>
  <borderColor>#f47b00</borderColor>
  <showvalue>true</showvalue>
  <flatstyle>true</flatstyle>
  <integerMode>false</integerMode>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>24</x>
  <y>119</y>
  <width>95</width>
  <height>50</height>
  <uuid>{2cea6dc5-e69f-4042-a4ea-46cface19e43}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Mod. Midinote</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>Liberation Sans</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>241</r>
   <g>122</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBKnob" version="2">
  <objectName>feedback</objectName>
  <x>134</x>
  <y>19</y>
  <width>100</width>
  <height>100</height>
  <uuid>{c4b6071d-88bf-451a-9568-110640ac735e}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.49000000</value>
  <mode>lin</mode>
  <mouseControl act="">continuous</mouseControl>
  <resolution>0.01000000</resolution>
  <randomizable group="0">false</randomizable>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <textcolor>#f37b00</textcolor>
  <border>1</border>
  <borderColor>#f47b00</borderColor>
  <showvalue>true</showvalue>
  <flatstyle>true</flatstyle>
  <integerMode>false</integerMode>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>139</x>
  <y>119</y>
  <width>95</width>
  <height>31</height>
  <uuid>{54f75bbf-7d81-4bbe-b60a-f5531ee83730}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Feedback</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>Liberation Sans</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>241</r>
   <g>122</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBCheckBox" version="2">
  <objectName>diode</objectName>
  <x>359</x>
  <y>19</y>
  <width>40</width>
  <height>40</height>
  <uuid>{174798b9-e1f6-41a4-9218-3f0615298d18}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <selected>true</selected>
  <label/>
  <pressedValue>1</pressedValue>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>399</x>
  <y>24</y>
  <width>95</width>
  <height>31</height>
  <uuid>{b189b7d6-b223-4886-ad7c-f97e12fdd35e}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Diode</label>
  <alignment>left</alignment>
  <valignment>center</valignment>
  <font>Liberation Sans</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>241</r>
   <g>122</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBCheckBox" version="2">
  <objectName>oversample</objectName>
  <x>359</x>
  <y>64</y>
  <width>40</width>
  <height>40</height>
  <uuid>{155ab1d9-3b3d-4af4-afea-3079c08c79cc}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <selected>true</selected>
  <label/>
  <pressedValue>1</pressedValue>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>399</x>
  <y>69</y>
  <width>140</width>
  <height>31</height>
  <uuid>{40e11ba9-381e-4aa0-bdbc-17d726043784}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Oversample (2x)</label>
  <alignment>left</alignment>
  <valignment>center</valignment>
  <font>Liberation Sans</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>241</r>
   <g>122</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBKnob" version="2">
  <objectName>nonlinear</objectName>
  <x>249</x>
  <y>19</y>
  <width>100</width>
  <height>100</height>
  <uuid>{f6f711dd-c93a-4db7-8483-6d84f6d614a2}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.63000000</value>
  <mode>lin</mode>
  <mouseControl act="">continuous</mouseControl>
  <resolution>0.01000000</resolution>
  <randomizable group="0">false</randomizable>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <textcolor>#f37b00</textcolor>
  <border>1</border>
  <borderColor>#f47b00</borderColor>
  <showvalue>true</showvalue>
  <flatstyle>true</flatstyle>
  <integerMode>false</integerMode>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>254</x>
  <y>119</y>
  <width>95</width>
  <height>50</height>
  <uuid>{538a713d-805c-47a9-b626-c22f06771d2c}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Non-linearities</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>Liberation Sans</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>241</r>
   <g>122</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>note</objectName>
  <x>34</x>
  <y>177</y>
  <width>80</width>
  <height>36</height>
  <uuid>{cc33e535-60c1-44e6-b835-f1991d685c82}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>4F#</label>
  <alignment>center</alignment>
  <valignment>center</valignment>
  <font>Liberation Sans</font>
  <fontsize>24</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>170</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
