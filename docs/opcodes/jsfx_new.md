# jsfx_new

## Abstract

Instantiates a jsfx script 


## Description

`jsfx_new` instantiates and compiles a jsfx script (at init time). It returns
a handle which can be used to modify control values and perform audio io. It
is also possible to instantiate and play a script with only one opcode
via `jsfx`.

!!! note "jsfx"

    `jsfx` is an audio programming language implemented pritarily as part of the DAW `REAPER`. 
    It is a scriptiong language with a built-in jit compiler which translates it to 
    machine code on the fly. It allows the user to operate at the sample level (like 
    defining an udo with `setksmps 1` but more efficient). It is around 2x to 2.5x slower
    than hand-coded C.
    
!!! note "jsfx input / output"

    A jsfx script has a certain number of audio input / output channels, and a series of 
    "sliders", which are parameters operating at control rate. A script can also use
    these sliders to send control values, which can be read in csound via [jsfx_getslider]
    See https://www.reaper.fm/sdk/js/js.php for more information about the syntax, etc.

## Syntax

    ihandle jsfx_new Spath
    
### Arguments

* **Spath**: the path to the jsfx script. Either an absolute path, a relative path to the 
  .csd file, or a filename alone, in which case it will be searched first in the current dir
  and in $SSDIR, if defined.

### Output

* **ihandle**: a handle to the jsfx plugin created, which allows to operate on it later,
  for instance to perform audio io via [jsfx_play]

### Execution Time

* Init

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

i1 0.1 3600

</CsScore>
</CsoundSynthesizer>



```


## See also

* [jsfx]
* [jsfx_play]
* [jsfx_setslider]
* [jsfx_getslider]

## Credits

Eduardo Moguillansky, 2019

Uses the open-source implementation of the jsfx language by Pascal Gauthier et al. Based heavily on
the pd external `jsfx~`.

https://github.com/asb2m10/jsusfx


[jsfx_new]: jsfx_new.md
[jsfx]: jsfx.md
[jsfx_play]: jsfx_play.md
[jsfx_getslider]: jsfx_getslider.md
[jsfx_setslider]: jsfx_setslider.md
