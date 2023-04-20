<CsoundSynthesizer>
<CsOptions>
-odac           
   
</CsOptions>

<CsInstruments>

/*
    Example file for lfnoise

    lfnoise generates a random value between 0-1 at the given
    frequency. If kinterp=1, then values are interpolated
    Otherwise, they are held until next value

*/

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1


instr 1
  kt eventtime
  kfreq   bpf kt, 0, 1, 10, 20, 20, 200, 30, 200, 40, 500, 50, 2000
  kinterp = round(k(vco2:a(0.5, 1/5, 2, 0.5) + 0.5))
  if metro(12) == 1 then
    println "freq: %.1f, interp: %.1f", kfreq, kinterp
  endif
  kgain = 0.5
  aout lfnoise kfreq, kinterp
  aout *= interp(kgain)    
  outall aout
endin

</CsInstruments>

<CsScore>
i1 0 50

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
 <bgcolor mode="background">
  <r>240</r>
  <g>240</g>
  <b>240</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
