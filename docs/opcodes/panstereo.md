# panstereo

## Abstract

Stereo signal balancer 


## Description

Equal power panning balances two channels. By panning **from left
(pos=0) to right (pos=1)** you are decrementing the level of the left
channel from 1 to 0 taking the square root of the linear scaling
factor, while at the same time incrementing the level of the right
channel from 0 to 1 using the same curve. In the center position
(pos=0.5) this results in a level for both channels of `sqrt(0.5)` (`~=0.707`
or **-3dB**). The output of panstereo remains a stereo signal. This is a 
port of Supercollider's `Balance2` ugen.

!!! note

    `kpan` is defined between 0 (left) and 1 (right) to make it coherent with
    opcodes like `pan2`, which also use this range. This differs from the original
    implementation in Supercollider, which uses a pan value of -1 to 1.
    Notice that even if `kpan` is a scalar (k-) variable, it is interpolated internaly
    to prevent discontinuities ("zipper" noise).

## Syntax

```csound
aoutL, aoutR panstereo aL, aR, kpan, klevel=1
```

### Arguments

* **aL**: left input 
* **aR**: right input
* **kpan**: panning position, **between 0 (left) and 1 (right)**
* **klevel**: control rate level input (defaults to 1)

### Output

* **aoutL**: left output
* **aoutR**: right output

### Execution Time

* Performance

-----------------

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



```

------------------

## See also

* [pan2](https://csound.com/docs/manual/pan2.html)
* [original implementation](https://doc.sccode.org/Classes/Balance2.html)
* [bpf](https://csound.com/docs/manual/bpf.html)
* [Panning and Spatialization](http://write.flossmanuals.net/csound/b-panning-and-spatialization/)

## Credits

Eduardo Moguillansky, 2021
