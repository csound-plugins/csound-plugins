# perlin3

## Abstract

gradient noise sound generator

## Description

Perlin noise is a type of gradient noise devised by Ken Perlin, and commonly used to add texture
to objects rendered in computer graphics. It is deterministic: for a given input co-ordinate it
always returns the same output value. To make audible noise you will typically need to define
some trajectory through the co-ordinate space.

`perlin3` is a csound port of Supercollider's `Perlin3`


## Syntax

```csound

aout  perlin3 ax, ay, az
kout  perlin3 kx, ky, kz
```

## Arguments

* `kx`, `ky`, `kz`: the coordinates of a point in 3D space. In the current implementation the space wraps at 255 so any value is actually possible.

## Output

* `aout` / `kout`: the perlin noise corresponding to the given coordinates

## Execution Time

* Performance

## Examples

```csound


<CsoundSynthesizer>

<CsInstruments>
/* to be run in csoundqt >= 0.9.8 */

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

; an empty table to plot
giplot1 ftgen 0, 0, 2000, 2, 0
giplot2 ftgen 0, 0, 2000, 2, 0

opcode tabplot, 0, Sik
	Schan, itab, kvalue xin
	; init
	outvalue Schan, itab
	
	tablew kvalue, accum:k(1), itab, 0, 0, 1
	
	; update the plot
	if metro(30) == 1 then
		outvalue Schan, k(-1)
	endif
endop

instr perlin3
  kspeed = line:k(0.1, 10, 2)
  ax = accum:a(kspeed/sr)
  az = ax*0.5
  aper1 = perlin3(ax, a(0), az)
  aper2 = perlin3(a(0), ax, az)
  asig = pinker() * 0.4
  
  ; remap to 0-1
  aper1 = (aper1 + 1) * 0.5
  aper2 = (aper2 + 1) * 0.5
  
  ilagtime = 0.1
  a1 = asig*lag(aper1, ilagtime)
  a2 = asig*lag(aper2, ilagtime)
  a1, a2 reverbsc a1, a2, 0.92, 12000
  outch 1, a1, 2, a2
  
  ; plot the trajectory in csoundqt
  tabplot "plot1", giplot1, aper1[0]
  tabplot "plot2", giplot2, aper2[0]
   
endin
  

</CsInstruments>

<CsScore>
i "perlin3" 0 300


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
 <bsbObject version="2" type="BSBTableDisplay">
  <objectName>plot1</objectName>
  <x>6</x>
  <y>11</y>
  <width>500</width>
  <height>150</height>
  <uuid>{b75d16b1-05b3-4358-974e-f2dfa85d3d34}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <color>
   <r>255</r>
   <g>193</g>
   <b>3</b>
  </color>
  <range>0.00</range>
 </bsbObject>
 <bsbObject version="2" type="BSBTableDisplay">
  <objectName>plot2</objectName>
  <x>5</x>
  <y>170</y>
  <width>500</width>
  <height>150</height>
  <uuid>{9a8bac1d-2234-450c-8807-37f9902bfb74}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <color>
   <r>255</r>
   <g>193</g>
   <b>3</b>
  </color>
  <range>0.00</range>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>



```


## See also

* [Supercollider's Perlin3](https://doc.sccode.org/Classes/Perlin3.html)

## Credits

Eduardo Moguillansky, 2020
