<CsoundSynthesizer>
<CsOptions>
-odac           
   
</CsOptions>

<CsInstruments>

/* 

    sigmdrive: a sigmoid distortion

    aout sigmdrive ain, kdrive, kmode=0

    kdrive: how much distortion (range 0-inf)
    kmode: 0 = tanh, 1 = pow
    
*/

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

chn_k "drive", "r"
chn_k "mode", "r"
chn_k "midinote", "r"

chnset 10, "drive"
chnset 1, "mode"

instr 1
	kmidinote chnget "midinote"
	kdrive chnget "drive"
	kmode chnget "mode"		
	ain oscili 0.2, lag(mtof:k(kmidinote), 0.2)
	aout sigmdrive ain, port:k(kdrive, 0.05), kmode
	dispfft aout, 1/20, 2048
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
 <bgcolor mode="background">
  <r>38</r>
  <g>41</g>
  <b>45</b>
 </bgcolor>
 <bsbObject version="2" type="BSBKnob">
  <objectName>drive</objectName>
  <x>140</x>
  <y>0</y>
  <width>80</width>
  <height>80</height>
  <uuid>{72f1554b-6adf-45e3-998d-4d22945dd484}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <minimum>0.00000000</minimum>
  <maximum>40.00000000</maximum>
  <value>2.50000000</value>
  <mode>lin</mode>
  <mouseControl act="">continuous</mouseControl>
  <resolution>0.01000000</resolution>
  <randomizable group="0">false</randomizable>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <textcolor>#f67c00</textcolor>
  <border>1</border>
  <borderColor>#f57c00</borderColor>
  <showvalue>true</showvalue>
  <flatstyle>true</flatstyle>
  <integerMode>false</integerMode>
 </bsbObject>
 <bsbObject version="2" type="BSBDropdown">
  <objectName>mode</objectName>
  <x>265</x>
  <y>50</y>
  <width>80</width>
  <height>30</height>
  <uuid>{092a8810-6562-4cf4-8c3b-6c68ecfdfc1b}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <bsbDropdownItemList>
   <bsbDropdownItem>
    <name>tan0</name>
    <value>0</value>
    <stringvalue/>
   </bsbDropdownItem>
   <bsbDropdownItem>
    <name>pow</name>
    <value>1</value>
    <stringvalue/>
   </bsbDropdownItem>
  </bsbDropdownItemList>
  <selectedIndex>1</selectedIndex>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBGraph">
  <objectName>graph</objectName>
  <x>25</x>
  <y>115</y>
  <width>709</width>
  <height>297</height>
  <uuid>{7bdf5122-fdcc-4c85-9f30-0ba8ede6f29a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <value>0</value>
  <objectName2/>
  <zoomx>1.00000000</zoomx>
  <zoomy>1.00000000</zoomy>
  <dispx>1.00000000</dispx>
  <dispy>1.00000000</dispy>
  <modex>lin</modex>
  <modey>lin</modey>
  <showSelector>false</showSelector>
  <showGrid>true</showGrid>
  <showTableInfo>false</showTableInfo>
  <showScrollbars>true</showScrollbars>
  <enableTables>false</enableTables>
  <enableDisplays>true</enableDisplays>
  <all>true</all>
 </bsbObject>
 <bsbObject version="2" type="BSBScope">
  <objectName>1</objectName>
  <x>25</x>
  <y>415</y>
  <width>350</width>
  <height>150</height>
  <uuid>{a6d87942-d64d-45c1-a0b8-9b466f92c36a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <value>-255.00000000</value>
  <type>scope</type>
  <zoomx>1.00000000</zoomx>
  <zoomy>1.00000000</zoomy>
  <dispx>1.00000000</dispx>
  <dispy>1.00000000</dispy>
  <mode>0.00000000</mode>
  <triggermode>TriggerUp</triggermode>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>169</x>
  <y>-1</y>
  <width>80</width>
  <height>25</height>
  <uuid>{7c15df99-3bb7-497e-b509-60a6d5f866db}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Drive</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>Liberation Sans</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>245</r>
   <g>124</g>
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
 <bsbObject version="2" type="BSBKnob">
  <objectName>midinote</objectName>
  <x>25</x>
  <y>0</y>
  <width>80</width>
  <height>80</height>
  <uuid>{5736f449-308c-4301-a80b-70e77923d957}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <minimum>24.00000000</minimum>
  <maximum>100.00000000</maximum>
  <value>51.41320000</value>
  <mode>lin</mode>
  <mouseControl act="">continuous</mouseControl>
  <resolution>0.01000000</resolution>
  <randomizable group="0">false</randomizable>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <textcolor>#f67c00</textcolor>
  <border>1</border>
  <borderColor>#f57c00</borderColor>
  <showvalue>true</showvalue>
  <flatstyle>true</flatstyle>
  <integerMode>false</integerMode>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>25</x>
  <y>80</y>
  <width>80</width>
  <height>25</height>
  <uuid>{41a3d82b-b370-4155-9af8-88eac21bd391}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Midinote</label>
  <alignment>center</alignment>
  <valignment>top</valignment>
  <font>Liberation Sans</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>245</r>
   <g>124</g>
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
