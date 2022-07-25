# weightedsum

## Abstract

Weighted sum of multiple 1D arrays, or the rows of a 2D array

## Description

Given a 2D array, `weightedsum` takes an array of factors (the weights),
one for each row, and multiplies each element in a row by its factor adding
all 1D arrays together to produce a weighted sum of such arrays. If the
2D array has a shape `(numrows, numcolumns)` then the weights array
should be a 1D array of size `numrows` and the output array is also
a 1D array of size `numcolumns`.

In the following example `weightedsum` performs an average between the 2nd
and the 4th row, effectively a vowel sound halfway between E and O

Notice that the absolute value of the weights is not really important, since these
are relative weights. The same result would be achieved with `kweights[] fillarray 0, 0.5, 0, 0.5,0`

```csound

iformantFreqs[] fillarray 800, 1150, 2900, 3900, 4950, \  ; A
                          350, 2000, 2800, 3600, 4950, \  ; E
                          270, 2140, 2950, 3900, 4950, \  ; I
                          450, 800, 2830, 3800, 4950,  \  ; O
                          325, 700, 2700, 3800, 4950      ; U

reshapearray iformantFreqs, 5, 5
kweights[] fillarray 0, 1, 0, 1, 0
kformants[] weightedsum iformantFreqs, kweights

-> [400, 1400, 2815, 3700, 4950]
```

## Syntax

```csound
kout[]  weightedsum kmatrix[], kweights[]
kout[]  weightedsum imatrix[], kweights[]
```
    
## Arguments

* **kmatrix** / **imatrix**: a 2D array. Each element in a row will be multiplied by the row's
    weight (given in `kweights`) and all rows will be summed together
* **kweights**: the weight of each row of `kmatrix` (normally a value between 0-1). This
    should be a 1D array with a size equal to the number of rows in `kmatrix`
        
## Output

* **kout**: a 1D array of size `numcolumns` with the weighted sum of the input array weighted
    by the given weights.
  
## See Also

* [interp1d](interp1d.md)


## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 4
0dbfs = 1

giformantFreqs[] fillarray 800, 1150, 2900, 3900, 4950, \  ; A
                           350, 2000, 2800, 3600, 4950, \  ; E
                           270, 2140, 2950, 3900, 4950, \  ; I
                           450, 800, 2830, 3800, 4950,  \  ; O
                           325, 700, 2700, 3800, 4950      ; U
                         

                           
giformantDbs[] fillarray   0, -6, -32, -20, -50, \
                           0, -20, -15, -40, -56, \
                           0, -12, -26, -26, -44, \
                           0, -11, -22, -22, -50, \
                           0, -16, -35, -40, -60
                           
giformantBws[] fillarray   80, 90, 120, 130, 140, \
                           60, 100, 120, 150, 200, \
                           60, 90, 100, 120, 120, \
                           40, 80, 100, 120, 120, \
                           50, 60, 170, 180, 200


giformantAmps[] maparray giformantDbs, "ampdb"

reshapearray giformantFreqs, 5, 5
reshapearray giformantAmps, 5, 5
reshapearray giformantBws, 5, 5

instr 10
  kmidi = p4
  kamp = p5
  kx = p6
  ky = p7
  kmidi = lag:k(kmidi, 0.3)
  
  kvibfreq linseg 0, 0.3, 0, 2.5, 4.5
  ivibsemi = 0.1
  kvib oscil ivibsemi/2, kvibfreq
  kvib -= ivibsemi / 2
  kpitch = kmidi + kvib
  kfreq = lag(mtof(kpitch), 0.2)
  
  ; asource = mpulse:a(kamp*10, 1/kfreq)
  asource vco2 kamp, kfreq
  asource = butterlp:a(asource, 4000)
  
  ;                   x    y    weight
  kcoords[] fillarray 0,   0,   1,      \    ; A
                      0.5, 0.5, 0.4,    \    ; E
                      1,   0,   1,      \    ; I
                      0,   1,   1,      \    ; O
                      1,   1,   1            ; U
  kweights[] init 5
  kformantFreqs[] init 5
  kformantBws[] init 5
  kformantAmps[] init 5
         
  if changed:k(kx, ky) == 1 then
    kweights presetinterpw kx, ky, kcoords, 0.25
    printarray kweights
    kformantFreqs weightedsum giformantFreqs, kweights
    kformantBws   weightedsum giformantBws, kweights
    kformantAmps  weightedsum giformantAmps, kweights
  
  endif
  kformantFreqs poly 5, "lag", kformantFreqs, 0.1
  kformantAmps poly 5, "lag", kformantAmps, 0.1
    
  aformants[] poly 5, "resonx", asource, kformantFreqs, kformantBws, 2, 2
  ; aformants[] poly 5, "butterbp", asource, kformantFreqs, kformantBws
  aformants *= kformantAmps
  asum sumarray aformants
  asum *= a(kamp)
  asum *= linsegr(0, 0.1, 1, 0.1, 0)
  ; asum *= 0.1
  ; dispfft asum, 0.05, 4096
  dispfft asource, 0.05, 4096
  outch 1, asum, 3, asource
endin

instr 20
  ip1 = p4
  kt = timeinsts()
  kmidi bpf kt, 0, 60, 10, 60, 18, 36
  kamp = 0.1
  kradius linseg 0.5, 10, 0.5, 10, 0.1
  kfreq = 1/10
  kx = oscil:k(kradius, kfreq)+0.5
  ky = oscil:k(kradius, kfreq, -1, 3.1415/4)+0.5
  pwrite ip1, 4, kmidi, 5, kamp, 6, kx, 7, ky
  if metro(30) == 1 then
    outvalue "x", kx
    outvalue "y", ky
    outvalue "note", mton(round(kmidi*2)/2)
  endif
endin

idur = 20
schedule 10.01, 0, idur, 48, 0.5, 0, 1, 0
schedule 20, 0, idur, 10.01

</CsInstruments>
<CsScore>

</CsScore>
</CsoundSynthesizer>











































<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>0</x>
 <y>0</y>
 <width>833</width>
 <height>631</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="background">
  <r>40</r>
  <g>40</g>
  <b>40</b>
 </bgcolor>
 <bsbObject version="2" type="BSBGraph">
  <objectName>graph1</objectName>
  <x>5</x>
  <y>9</y>
  <width>828</width>
  <height>313</height>
  <uuid>{a386abff-a2e9-4fd5-a2a1-044b268f027e}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <value>0</value>
  <objectName2/>
  <zoomx>4.00000000</zoomx>
  <zoomy>1.00000000</zoomy>
  <dispx>1.00000000</dispx>
  <dispy>1.00000000</dispy>
  <modex>lin</modex>
  <modey>lin</modey>
  <showSelector>false</showSelector>
  <showGrid>true</showGrid>
  <showTableInfo>true</showTableInfo>
  <showScrollbars>false</showScrollbars>
  <enableTables>false</enableTables>
  <enableDisplays>true</enableDisplays>
  <all>true</all>
 </bsbObject>
 <bsbObject version="2" type="BSBController">
  <objectName>x</objectName>
  <x>171</x>
  <y>332</y>
  <width>300</width>
  <height>300</height>
  <uuid>{f50d2326-9e70-41bf-a9d2-ea39b20ad45b}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <description/>
  <objectName2>y</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.99604966</xValue>
  <yValue>0.54824522</yValue>
  <type>point</type>
  <pointsize>20</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <bordermode>border</bordermode>
  <borderColor>#007800</borderColor>
  <color>
   <r>0</r>
   <g>234</g>
   <b>0</b>
  </color>
  <randomizable mode="both" group="0">false</randomizable>
  <bgcolor>
   <r>0</r>
   <g>80</g>
   <b>0</b>
  </bgcolor>
  <bgcolormode>true</bgcolormode>
 </bsbObject>
 <bsbObject version="2" type="BSBDisplay">
  <objectName>note</objectName>
  <x>96</x>
  <y>332</y>
  <width>64</width>
  <height>36</height>
  <uuid>{ede113aa-8dda-4374-ab74-995b45a818df}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>4C</label>
  <alignment>left</alignment>
  <valignment>center</valignment>
  <font>Liberation Sans</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </color>
  <bgcolor mode="background">
   <r>24</r>
   <g>24</g>
   <b>24</b>
  </bgcolor>
  <bordermode>false</bordermode>
  <borderradius>5</borderradius>
  <borderwidth>0</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>50</x>
  <y>336</y>
  <width>43</width>
  <height>26</height>
  <uuid>{1b1e8fcb-efcc-4236-a315-e07e13fafec0}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <label>Pitch</label>
  <alignment>right</alignment>
  <valignment>center</valignment>
  <font>Liberation Sans</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>239</r>
   <g>239</g>
   <b>239</b>
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
  <objectName/>
  <x>483</x>
  <y>332</y>
  <width>350</width>
  <height>150</height>
  <uuid>{735224e3-fc71-454b-9aee-f857e8ea1b89}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <description/>
  <value>3.00000000</value>
  <type>scope</type>
  <zoomx>8.00000000</zoomx>
  <zoomy>2.00000000</zoomy>
  <dispx>1.00000000</dispx>
  <dispy>1.00000000</dispy>
  <mode>0.00000000</mode>
  <triggermode>TriggerUp</triggermode>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>



```
