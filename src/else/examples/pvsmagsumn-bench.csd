<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

/* Example file for pvsmagsumn

ktotalmag pvsmagsum fsig, inumbins, kminfreq=0, kmaxfreq=sr/2

  Sum the magnitudes of the loudest bins between given frequencies
  
  Can be used to calculate how much energy is concentrated on the
  loudest `inumbins` bins. The ratio between this value and the
  sum of all magnitudes is a good indicator of how spread out
  the energy of the signal is across the frequency spectrum, thus
  serving as a good measure for the noisyness of the signal. 
  A noisy signal will tend to have energy spread more evenly across
  the spectrum, while a musical or speech signal will concentrate
  that signal along a limited range of bins. 
  This opcode combines in one the use of pvstrace and pvsmagsum
  making it more efficient for the use cases where the spectrum
  of the selected bins is not actually needed.
  
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
  ifftsize = 2048
  asig = picksource(ksource, asig1, asig2, asig3, asig4, asig5)
  fsig = pvsanal(asig, ifftsize, 256, ifftsize, 0)
  ; for benchmarking
  kpeakyness = pvsmagsumn(fsig, -10, 50)
  kpeakyness = pvsmagsumn(fsig, -10, 50)
  kpeakyness = pvsmagsumn(fsig, -10, 50)
  kpeakyness = pvsmagsumn(fsig, -10, 50)
      
  kpeakyness = lag(kpeakyness, 0.2)
  ; if mtro(10) == 1 then
  ; 	println "source: %s, peakyness: %d", Snames[ksource], kpeakyness*100
  ; endif
  outch 1, asig
endin

</CsInstruments>

<CsScore>
i1 0 20

</CsScore>
</CsoundSynthesizer>
