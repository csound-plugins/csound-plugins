# jsfx_setslider

## Abstract

Sets the slider values of a jsfx script


## Description

With `jsfx_setslider` it is possible to set the values of any number of sliders
defined in a jsfx script, mostly prior to calling `jsfx_play`.
A jsfx plugin defines a series of up to 64 sliders, which are control parameters
used by the script. A slider definition in a jsfx script has the form:

    slider3:1.5<0,4,0.01>Compression Ratio
    
This defines a control parameter with default value `1.5`, between `0` and `4`,
with a precission (an increment) of `0.01` and a label "Copression Ratio". 

```csound
kcomprat = 2.5
jsfx_setslider ihandle, 3, kcomprat
```
    
The code above will set the slider #3 (the index passed corresponds with the 
slider number) to the value of `kcomprat`
The value of any slider can be read via `jsfx_getvalue` (only one value at a time).

!!! Note "many sliders"

    It is possible to call jsfx_setslider with any number of sliders. It is not
    necessary to set all the sliders defined in the script. Any slider
    which is not set via jsfx_setslider retains its default value. A value will 
    always be confined to the range in the slider definition and will also be 
    quantized to the increment in the slider definition. To disable any quantization,
    set the increment to 0 in the jsfx script

!!! Note "jsfx"

    `jsfx` is an audio programming language implemented primarily as part of the DAW `REAPER`. 
    It is a scriptiong language with a built-in compiler which translates it to 
    machine code. It allows the user to operate at the sample level (like 
    defining an udo with `setksmps 1` but more efficient). It is around 2x to 2.5x slower
    than hand-coded C.
    See https://www.reaper.fm/sdk/js/js.php for more information about the syntax, etc.


## Syntax

```csound

jsfx_setslider ihandle, id1, kval1 [, id2, kval2, id3, kval3, ...]
```    
    
### Arguments

* **ihandle**: the handle created via [jsfx_new] or [jsfx]
* **idx**, **kvalx**: a jsfx script allows to define up to 64 control parameters, which are
  called `slider`s. Each slider has an idx (starting from 1) and a value. Here you can control
  as many sliders as you need. Each slider consists of a pair of values, an id 
  identifying the slider (this corresponds to the sliderx value in the jsfx script) and the
  value itself (a k- value)

### Output

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

;; This is the example file for the opcodes jsfx_new, jsfx_play and jsfx_setslider

gisnd ftgen 0, 0, 0, -1, "bourre-fragment-1.flac", 0, 0, 1


FLpanel "jsfx - tubeharmonics", 400, 500, 50, 50
FLcolor 150, 100, 150, 200, 100, 250

iw, ih = 300, 30
iline = ih * 2
iy, ix = ih, ih * 0.5
;                                    min max  exp
gkwhich, i0 FLslider "source",       0,    2,   0, 3,  -1, iw, ih, ix, iy
iy += iline
iv1 FLvalue "", 50, 30, 322, iy
gkeven, i1 FLslider "even harmonics",0,    1,   0, 3, iv1, iw, ih, ix, iy
iy += iline
iv2 FLvalue "", 50, 30, 322, iy
gkodd,  i2 FLslider "odd harmonics", 0,    1,   0, 3, iv2, iw, ih, ix, iy
iy += iline
iv3 FLvalue "", 50, 30, 322, iy
gkflct, i3 FLslider "fluctuation",   0,    1,   0, 3, iv3, iw, ih, ix, iy
iy += iline
iv4 FLvalue "", 50, 30, 322, iy
gkinpt, i4 FLslider "Input (dB)",  -12,  12,  0, 3, iv4, iw, ih, ix, iy
iy += iline
iv5 FLvalue "", 50, 30, 322, iy
gkout,  i5 FLslider "Output (dB)", -12,  12,  0, 3, iv5, iw, ih, ix, iy
iy += iline
FLcolor 150, 100, 150, 200, 200, 100
gkdump, i6 FLbutton "Dump variables", 1, 0, 2, iw/2, ih, ix, iy, -1 
FLpanelEnd
FLrun

FLsetVal_i 0, i0
FLsetVal_i 0.3, i1
FLsetVal_i 0.3, i2
FLsetVal_i 0.1, i3
FLsetVal_i 0, i4
FLsetVal_i 0, i5
FLsetVal_i 0, i6


opcode loopsamp, a, i
  ift xin
  iloopend = nsamp(ift) / sr
  asig flooper2 1, 1, 0, iloopend, 0.1, ift
  xout asig
endop

opcode select3, a, kaaa
  kwhich, a1, a2, a3 xin
  if(kwhich < 1) then
    asig = a1*(1-kwhich) + a2*kwhich
  else
    asig = a2*(2-kwhich) + a3*(kwhich-1)
  endif
  xout asig
endop

instr 1
  a1 loopsamp gisnd
  a3 vco2 0.5, ntof:i("3C")
  a2 oscili 0.5, 1000
  asig select3 gkwhich, a1, a2, a3
  ihandle jsfx_new "tubeharmonics.jsfx"
  print ihandle
  jsfx_setslider ihandle, 1, gkeven, 2, gkodd, 3, gkflct, 4, gkinpt, 5, gkout
  aout jsfx_play ihandle, asig
  if gkdump == 1 then
    jsfx_dump ihandle, metro(4)
  endif
  k7 jsfx_getslider ihandle, 7
  printf "k7: %f \r", changed2(k7), k7
  outs aout, aout
endin

</CsInstruments>

<CsScore>

i1 0 3600

</CsScore>
</CsoundSynthesizer>



```


## See also

* [jsfx]
* [jsfx_new]
* [jsfx_play]
* [jsfx_getslider]

## Credits

Eduardo Moguillansky, 2019

Uses the open-source implementation of the jsfx language by Pascal Gauthier et al. Based heavily on
the pd external `jsfx~`.

https://github.com/asb2m10/jsusfx

[jsfx]: jsfx.md
[jsfx_new]: jsfx_new.md
[jsfx_play]: jsfx_play.md
[jsfx_getslider]: jsfx_getslider.md
