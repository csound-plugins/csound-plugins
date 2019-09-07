# jsfx

## Abstract

Instantiates and runs a jsfx script 


## Description

`jsfx` allows to instantiate and run a jsfx audio plugin within csound. 

There are two ways to run a jsfx script in csound. The simplest way is implemented
in this plugin, `jsfx`, which allows to compile and control a jsfx plugin in one
opcode. As an alternative, it is also possible to decouple these actions, first 
compiling a script (see [jsfx_new]) and then calling `jsfx_setslider` and `jsfx_play`.
Afterwords, slider values can be read via `jsfx_getslider`

!!! Note jsfx

    `jsfx` is an audio programming language implemented pritarily as part of the DAW `REAPER`. 
    It is a scriptiong language with a built-in jit compiler which translates it to 
    machine code on the fly. It allows the user to operate at the sample level (like 
    defining an udo with `setksmps 1` but more efficient). It is around 2x to 2.5x slower
    than hand-coded C.

    
!!! Note jsfx inputs and outputs

    A jsfx script has a certain number of audio input / output channels, and a series of 
    "sliders", which are parameters operating at control rate. A script can also use
    these sliders to send control values, which can be read in csound via [jsfx_getslider]
    See https://www.reaper.fm/sdk/js/js.php for more information about the syntax, etc.


## Syntax

    ihandle, aout1 [, aout2, ...]  jsfx Spath, ain1 [, ain2, ...] [, id0, kval1, id1, kval2, ...]
    
### Arguments

* **Spath**: the path to the jsfx script. Either an absolute path, a relative path to the 
  .csd file, or a filename alone, in which case it will be searched first in the current dir
  and in $SSDIR, if defined.
* **ain1**, **ain2**, etc: audio input channels. It is recommended that the number of input
  streams matches the number of channels expected in the plugin. If you pass less that the 
  expected channels, the rest will be zeroed, and if passing more, only the number of 
  audio channels expected by the plugin will actually be processed
* **idx**, **kvalx**: a jsfx script allows to define up to 64 control parameters, which are
  called `slider`s. Each slider has an idx (starting from 1) and a value. Here you can control
  as many sliders as you need. Each slider consists of a pair of values, an id (i- value) 
  identifying the slider (this corresponds to the sliderx value in the jsfx script) and the
  value itself (a k- value)

### Output

* **ihandle**: a handle to the jsfx plugin created, which allows to operate on it later,
  for instance to read slider values via [jsfx_getslider]
* **aout1**, **aout2**: audio output channels. It is recommended that the number of output
  streams matches the number of channels expected in the plugin. If you pass less that the 
  expected channels, the rest will be zeroed, and if passing more, only the number of 
  audio channels expected by the plugin will actually be processed

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

;; This is the example file for jsfx opcode

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
  ih, a1 jsfx "tubeharmonics.jsfx", asig, 1, gkeven, 2, gkodd, 3, gkflct, 4, gkinpt, 5, gkout
  if gkdump == 1 then
    jsfx_dump ih, metro(4)
  endif
  outs a1, a1
endin


</CsInstruments>

<CsScore>

i1 0 3600

</CsScore>
</CsoundSynthesizer>

```


## See also

* [jsfx_new]
* [jsfx_play]
* [jsfx_setslider]
* [jsfx_getslider]

## Credits

Eduardo Moguillansky, 2019

Uses the open-source implementation of the jsfx language by Pascal Gauthier et al. Based heavily on
the pd external `jsfx~`.

https://github.com/asb2m10/jsusfx


[jsfx_new]: jsfx_new.md
[jsfx_play]: jsfx_play.md
[jsfx_getslider]: jsfx_getslider.md
[jsfx_setslider]: jsfx_setslider.md
