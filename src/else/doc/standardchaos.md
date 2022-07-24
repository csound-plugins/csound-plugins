# standardchaos

## Abstract

Standard map chaotic generator

## Description

`standardchaos` is a chaotic generator, the sound is generated with 
the following difference equations;

    y[n] = (y[n-1] + k * sin(x[n-1])) % 2pi;
    x[n] = (x[n-1] + y[n]) % 2pi;
    out = (x[n] - pi) / pi
    

## Syntax

```csound
    aout standardchaos krate, kk=1, ix=0.5, iy=0
```
    
### Arguments

* `krate`: from 0 to nyquist
* `kk`: a value for k in the above equation
* `ix`: initial value for x
* `iy`: initial value for y

### Output

* `aout`: audio output of the chaotic generator

### Execution Time

* Performance (audio)

## Examples

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

/* Example file for standardchaos opcode

	aout standardchaos krate, kk, ix=0.5, iy=0
	
	Standard map chaotic generator, the sound is generated with the difference equations;
    y[n] = (y[n-1] + k * sin(x[n-1])) % 2pi;
    x[n] = (x[n-1] + y[n]) % 2pi;
    out = (x[n] - pi) / pi;

*/

chn_k "rate", "r"
chn_k "kval", "r"

instr 1
	ix = 0.1
	iy = 0.0
	igain = 0.3
	krate chnget "rate"
	kk chnget "kval"
	
	aout standardchaos krate, kk, ix, iy
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
 <bsbObject version="2" type="BSBKnob">
  <objectName>rate</objectName>
  <x>20</x>
  <y>25</y>
  <width>120</width>
  <height>120</height>
  <uuid>{228ebadc-8fb9-4dee-b6fa-8216721da00d}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <minimum>0.00000000</minimum>
  <maximum>20000.00000000</maximum>
  <value>9656.00000000</value>
  <mode>lin</mode>
  <mouseControl act="">continuous</mouseControl>
  <resolution>0.01000000</resolution>
  <randomizable group="0">false</randomizable>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <textcolor>#f27b00</textcolor>
  <border>1</border>
  <borderColor>#1b1b1b</borderColor>
  <showvalue>true</showvalue>
  <flatstyle>true</flatstyle>
  <integerMode>true</integerMode>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>45</x>
  <y>145</y>
  <width>75</width>
  <height>41</height>
  <uuid>{189e690b-5322-423c-9288-7c213beb7889}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Rate</label>
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
 <bsbObject version="2" type="BSBKnob">
  <objectName>kval</objectName>
  <x>154</x>
  <y>25</y>
  <width>120</width>
  <height>120</height>
  <uuid>{feb1752a-9019-44c7-b654-2d0b1bb8b728}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <minimum>0.00000000</minimum>
  <maximum>20.00000000</maximum>
  <value>5.83000000</value>
  <mode>lin</mode>
  <mouseControl act="">continuous</mouseControl>
  <resolution>0.01000000</resolution>
  <randomizable group="0">false</randomizable>
  <color>
   <r>245</r>
   <g>124</g>
   <b>0</b>
  </color>
  <textcolor>#f27b00</textcolor>
  <border>1</border>
  <borderColor>#1b1b1b</borderColor>
  <showvalue>true</showvalue>
  <flatstyle>true</flatstyle>
  <integerMode>false</integerMode>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>179</x>
  <y>145</y>
  <width>75</width>
  <height>41</height>
  <uuid>{2d9d245b-763b-4aed-a795-66f3292fabc4}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>k</label>
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
 <bsbObject version="2" type="BSBScope">
  <objectName>1</objectName>
  <x>18</x>
  <y>239</y>
  <width>350</width>
  <height>150</height>
  <uuid>{70135fc8-22b9-41e3-a97a-eb185ddeee34}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <value>-255.00000000</value>
  <type>scope</type>
  <zoomx>2.00000000</zoomx>
  <zoomy>1.00000000</zoomy>
  <dispx>1.00000000</dispx>
  <dispy>1.00000000</dispy>
  <mode>0.00000000</mode>
  <triggermode>TriggerUp</triggermode>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>



```


## See also

* [crackle](crackle.md)
* [chuap](https://csound.com/docs/manual/chuap.html)
* [dust2](https://csound.com/docs/manual/dust2.html)

## Credits

Eduardo Moguillansky, 2019
(based on pd/else's `standard~` - https://github.com/porres/pd-else)
