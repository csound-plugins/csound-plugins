<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

; Example file for linexp.csd

/*

linexp

linear to exponential interpolation between two ranges

ky    linexp kx, kexp, ky0, ky1, kx0=0, kx1=1

*/

; Map a value within the range 1-3 to the range 0-10 with an exponent of 2.
instr 1
  kx line 1, p3, 3
  ky linexp kx, 2, 1, 3, 0, 10
  printks "kx: %f   ky: %f \n", 1/kr, kx, ky
endin


</CsInstruments>
<CsScore>
i 1 0   0.2

</CsScore>
</CsoundSynthesizer>
