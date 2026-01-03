<CsoundSynthesizer>

<CsInstruments>

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

; an empty table to plot
giplot1 ftgen 0, 0, 2000, 2, 0
giplot2 ftgen 0, 0, 2000, 2, 0

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
  
endin
  

</CsInstruments>

<CsScore>
i "perlin3" 0 20


</CsScore>
</CsoundSynthesizer>


