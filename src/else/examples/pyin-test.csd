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

gilen = int(30 * sr/ksmps) + 1
gitimes      ftgen 0, 0, gilen, -2, 0
gifreqs      ftgen 0, 0, gilen, -2, 0
giconfidence ftgen 0, 0, gilen, -2, 0
gienv        ftgen 0, 0, gilen, -2, 0

instr 1
  asig1 = buzz(0.1, 300, 7, -1)
  ; asig2 = diskin2("../../beosc/examples/colours-german-male-mono.flac", 1, 0, 1)[0]
  asig2 = diskin2("../../else/examples/finnegan01.flac", 1, 0, 1)[0]
  ; asig2 = diskin2("../../else/examples/voiceover-fragment-48k.flac", 1, 0, 1)[0] 
  Snames[] fillarray "sine", "speech", "speech", "speech"
  ksource init 0
  if metro(1/4) == 1 then
    ksource = (ksource + 1) % lenarray(Snames)
  endif
  asig = picksource(ksource, asig1, asig2, asig2, asig2)
  iframelen = 2048
  kpitch, kconf, kvoiced pyin asig, 60, 800, iframelen, 4, 0.992, 5
  ksound = schmitt(dbamp(rms(asig)),  -55, -65);
  kenv = schmitt:k(kconf, 0.2, 0.05) * ksound
  kicounter = eventcycles()
  tabw eventtime(), kicounter, gitimes
  tabw kpitch, kicounter, gifreqs
  tabw kconf, kicounter, giconfidence
  tabw kenv, kicounter, gienv
  outch 1, delay(asig, iframelen/sr)
  outch 2, vco2(0.1, kpitch) * a(kenv)
endin

instr 2
  savenpy "pyin-times.npy", gitimes
  savenpy "pyin-freq.npy", gifreqs
  savenpy "pyin-confidence.npy", giconfidence
  savenpy "pyin-env.npy", gienv
  turnoff
endin

</CsInstruments>

<CsScore>
i 1 0 12
i 2 12.1 0.1

</CsScore>
</CsoundSynthesizer>
