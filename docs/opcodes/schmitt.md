# schmitt

## Abstract

A schmitt trigger (a comparator with hysteresis). 


## Description

Implements a schmitt trigger, which is a comparator with hysteresis. Whenever the 
input is higher than `khigh`, output is 1 and stays 1 until input drops beneath
`klow`.

* Output is 1 if the input is higher than `khigh` if signal is increasing
* Output is 0 if the input is lower than `klow` if signal is decreasing

!!! Note 

    `schmitt` is particularly useful for implementing effects like a noise gate,
    to avoid fast opening and closing at the threshold. It can be further refined
    together with `lagud` to add attack / release times to the opening of the gate
    or with `trighold` to assure a minimum open time for the gate

Port of pd/else's `schmitt`

## Syntax

```csound

xout  schmitt xin, khigh, klow

```
    
### Arguments

* `xin`: input signal (k- or audio rate). The rate of `xin` must match the rate of `xout`
* `khigh`: high value of the comparator, output is 1 whenever input is higher than this
* `klow`: low value of the comparator, output is 0 whenever input is lower than this

### Output

* `xout`: output value of the comparator (0 or 1). Rate of xout is the same as xin

### Execution Time

* Performance (k or audio)

## Examples

![](assets/schmitt.gif)

```csound


<CsoundSynthesizer>
<CsOptions>
-odac           
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

/* Example file for schmitt opcode

	To be used inside csoundqt

	aout schmitt ain, khigh, klow
	kout schmitt kin, khigh, klow
	
	schmitt is a schmitt trigger (a gate with hysteresis), out is 1 if higher than khigh,
	0 if lower than klow

*/

chn_k "high", "r"
chn_k "low", "r"
chn_k "signal", "w"
chn_k "out", "w"

instr 1
	khigh chnget "high"
	klow chnget "low"
	
	ain = oscili:a(1, 0.25)*0.5+0.5
	aout schmitt ain, khigh, klow
	
	chnset ain[0], "signal"
	chnset aout[0], "out"
	
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
  <r>22</r>
  <g>22</g>
  <b>22</b>
 </bgcolor>
 <bsbObject version="2" type="BSBController">
  <objectName>high</objectName>
  <x>122</x>
  <y>39</y>
  <width>250</width>
  <height>50</height>
  <uuid>{9e532f2e-c12f-4bdf-9cae-a486e29fabc2}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2/>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.62800000</xValue>
  <yValue>0.00000000</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#00ff00</borderColor>
  <color>
   <r>0</r>
   <g>234</g>
   <b>0</b>
  </color>
  <randomizable mode="both" group="0">false</randomizable>
  <bgcolor>
   <r>0</r>
   <g>61</g>
   <b>0</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject version="2" type="BSBController">
  <objectName>low</objectName>
  <x>122</x>
  <y>100</y>
  <width>250</width>
  <height>50</height>
  <uuid>{c03eba29-3161-4d59-a2e7-3e143edea07f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2/>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.20400000</xValue>
  <yValue>0.00000000</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#fa5401</borderColor>
  <color>
   <r>255</r>
   <g>85</g>
   <b>0</b>
  </color>
  <randomizable mode="both" group="0">false</randomizable>
  <bgcolor>
   <r>65</r>
   <g>22</g>
   <b>0</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject version="2" type="BSBController">
  <objectName>signal</objectName>
  <x>122</x>
  <y>162</y>
  <width>250</width>
  <height>50</height>
  <uuid>{ca645074-a12d-45d9-865f-503de0fbb825}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2/>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.02908331</xValue>
  <yValue>0.00000000</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#01a6f9</borderColor>
  <color>
   <r>0</r>
   <g>170</g>
   <b>255</b>
  </color>
  <randomizable mode="both" group="0">false</randomizable>
  <bgcolor>
   <r>0</r>
   <g>43</g>
   <b>65</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject version="2" type="BSBController">
  <objectName>out</objectName>
  <x>122</x>
  <y>224</y>
  <width>250</width>
  <height>50</height>
  <uuid>{fddff123-5046-4832-9132-3829c553394a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2/>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.00000000</xValue>
  <yValue>0.00000000</yValue>
  <type>fill</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#fc017e</borderColor>
  <color>
   <r>255</r>
   <g>0</g>
   <b>127</b>
  </color>
  <randomizable mode="both" group="0">false</randomizable>
  <bgcolor>
   <r>76</r>
   <g>0</g>
   <b>38</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>30</x>
  <y>44</y>
  <width>85</width>
  <height>42</height>
  <uuid>{01a1fc08-6c00-4568-b299-8d4610dde144}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>High</label>
  <alignment>right</alignment>
  <valignment>center</valignment>
  <font>Liberation Sans</font>
  <fontsize>24</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>188</g>
   <b>155</b>
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
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>30</x>
  <y>103</y>
  <width>85</width>
  <height>42</height>
  <uuid>{4cbc717b-37d5-43cc-84b0-630c8a697ba1}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Low</label>
  <alignment>right</alignment>
  <valignment>center</valignment>
  <font>Liberation Sans</font>
  <fontsize>24</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>188</g>
   <b>155</b>
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
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>30</x>
  <y>166</y>
  <width>85</width>
  <height>42</height>
  <uuid>{2aaaf753-54af-4a64-8bc4-0eaf195ab7dd}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Signal</label>
  <alignment>right</alignment>
  <valignment>center</valignment>
  <font>Liberation Sans</font>
  <fontsize>24</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>188</g>
   <b>155</b>
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
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>28</x>
  <y>228</y>
  <width>85</width>
  <height>42</height>
  <uuid>{2cfa842f-03b1-454f-9a30-6be3126ed251}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Output</label>
  <alignment>right</alignment>
  <valignment>center</valignment>
  <font>Liberation Sans</font>
  <fontsize>24</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>188</g>
   <b>155</b>
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



```


## See also

* [lagud](https://csound.com/docs/manual/lagud.html)
* [trighold](https://csound.com/docs/manual/trighold.html)


## Credits

Eduardo Moguillansky, 2019
