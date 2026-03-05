<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

/* example file for pyin

Syntax:

kfreq, kconfidence, kvoiced pyin asig, iminfreq=60, imaxfreq=1000, ibufsize=2048, ioverlap=4, ktransprob=0.99, ibins=4, kdrift=5 

Args:
	* asig: audio signal
	* iminfreq: min. frequency for f0 (default=60)
	* imaxfreq: max. frequency for f0 (default=1000)
	* ibufsize: size of the analysis frame (default=2048)
	* ioverlap: overlapping frames. hopsize=bufsize/overlap (default=4)
	* ktransprob: hmm transition probability (default=0.99)
	* ibins: number of bins per semitone (default=4)
	* kdrift: pitch drift between frames, in semitones (default=5)

Output:
	* kfreq: detected frequency. Only valid if confidence is > ~0.4
	* kconfidence: detection confidence
	* kvoiced: is the sound voiced
	
*/


instr 1
  asig1 = oscili:a(0.5, 500)
  asig2 = buzz(0.1, 300, 7, -1)
  asig3 = pinker() * 0.1
  asig4 = diskin2("finnegan01.flac", 1, 0, 1)[0]
  Snames[] fillarray "sine ", "buzz ", "pink ", "finn "
  ksource init 0
  if metro(1/3) == 1 then
    ksource = (ksource + 1) % 4
  endif
  asig = picksource(ksource, asig1, asig2, asig3, asig4)
  kpitch, kconf, kvoiced pyin asig, 60, 1000, 2048, 4, 0.992, 5
  ksound = schmitt(dbamp(rms(asig)),  -45, -55);
  kenv = schmitt:k(kconf, 0.5, 0.3) * ksound;
  if metro(24) == 1 then
    printsk "Source: %d, %s, pitch: %f, conf: %f, voiced: %f, sound: %d\n", ksource, Snames[ksource], kpitch, kconf, kvoiced, ksound
  endif
  outch 1, asig
  outch 2, vco2(0.1, kpitch) * a(kenv)
endin

</CsInstruments>

<CsScore>
i1 1 20

</CsScore>
</CsoundSynthesizer>
