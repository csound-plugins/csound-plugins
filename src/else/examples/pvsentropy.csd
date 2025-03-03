<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

/* Example file for pvsmagsum

ktotalmag pvsmagsum fsig, kminfreq=0, kmaxfreq=sr/2

  Given an pv chain, sum all magnitudes of the bins between minfreq
  and maxfreq.

  Used together with pvstrace it can be determine how concentrated
  the energy of a signal is, giving an idea of how "voiced" or "pitched"
  it is. A noisy signal will tend to have energy spread more evenly across
  the spectrum, while a musical or speech signal will tend to concentrate
  that signal along a limited range of beans. By selecting the loudest
  bins via pvstrace and comparing the energy of the selected pv signal
  to the original signal this renders a quite relyable measurement of
  the "peakyness" of the signal.

*/

instr 1
  asig1 = oscili:a(0.5, 500)
  asig2 = buzz(0.1, 300, 7, -1)
  asig3 = pinker() * 0.1
  asig4 = unirand:a(2) - 1.0
  asig5 = diskin2("finnegan01.flac", 1, 0, 1)[0]
  Snames[] fillarray "sine ", "buzz ", "pink ", "white", "finn "
  ksource init 0
  if metro(0.5) == 1 then
    ksource = (ksource + 1) % 5
  endif
  ifftsize = 1024
  asig = picksource(ksource, asig1, asig2, asig3, asig4, asig5)
  fsig = pvsanal(asig, ifftsize, 256, ifftsize, 0)
  kentr = pvsentropy(fsig, 50)
  kentrnorm = bpf(kentr, 0, 0, 0.55, 0, 6, .15, 20, .5, 60, .9, 180, 1.) 
  if metro(30) == 1 then
    printsk "Source: %d, %s, entropy: %.3f (%d %%)\n", ksource, Snames[ksource], kentr, kentrnorm*100
  endif
  outch 1, asig
endin

</CsInstruments>

<CsScore>
i1 0 20

</CsScore>
</CsoundSynthesizer>
