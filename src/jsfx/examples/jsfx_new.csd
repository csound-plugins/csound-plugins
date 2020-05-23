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

gisnd ftgen 0, 0, 0, -1, "snd/bourre-fragment-1.flac", 0, 0, 1


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