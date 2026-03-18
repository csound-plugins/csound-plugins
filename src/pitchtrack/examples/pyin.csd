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

kfreq, kconfidence, kvoiced pyin asig, Sarg1, ivalue1, [Sarg2, ivalue2, ...]

Args:
	* asig: audio signal
	* Sarg1: an argument name, see below
	* ivalue1: the argument value

Output:
	* kfreq: detected frequency. Only valid if confidence is > ~0.4
	* kconfidence: detection confidence
	* kvoiced: is the sound voiced


Parameter          Range          Default
-----------------  ------------  -----------------
framesize          1024 - 8192    2048
hop                64 -           1/4 of framesize
fmin               20 -           60
fmax               20 -           1000
bins               2 - 50         10
transition_weight  0. - 1.        0.1
minrms             0. - 1.        0.
beta_a             1 - 3          2
beta_b             1 - 20         3
drift              10 -           100
voiced_obs_floor   0-1            0.
octave_cost        0.             0. - 0.5
subharmonic_tresh  4              1 - 8

*/


instr 1
  asig1 = oscili:a(0.5, 500)
  ; asig2 = diskin2("../../beosc/examples/colours-german-male-mono.flac", 1, 0, 1)[0]
  ; asig2 = diskin2("../../else/examples/finnegan01.flac", 1, 0, 1)[0]
  asig2 = diskin2("../../else/examples/voiceover-fragment-48k.flac", 1, 0, 1)[0]
  asig3 = buzz(0.1, 300, 7, -1)
  asig4 = pinker() * 0.1
  Snames[] fillarray "sine  ", "speech", "buzz  ", "pink  "
  ksource init 0
  if metro(1/3) == 1 then
    ksource = (ksource + 1) % 4
  endif
  ; asig = picksource(ksource, asig1, asig2, asig3, asig4)
  asig = asig2
  ; We compress the signal to get better detection
  asigpyin = compress2:a(asig, asig, -90, -40, -20, 4, 0.01, 0.5, 0.02)
  asigpyin *= 3
  kpitch, kconf, kvoiced pyin asigpyin, "framesize", 2048, "hop", 256, \
  	"fmin", 70, "fmax", 600, "transition_weight", 0.01, "beta_b", 1.6, \
  	"drift", 120, "voiced_obs_floor", 0.05
  ; akOOOOO
  apllpitch, aloc plltrack asigpyin, 0.05, 15, 0.15, 70, 600, 0.0001
  ksound = schmitt(dbamp(rms(asig)),  -45, -60);
  kvalid = schmitt:k(kconf, 0.3, 0.2) * schmitt:k(kpitch, 90, 75) * (1-schmitt:k(kpitch, 400, 300))
  kenv = kvalid * ksound;
  if metro(24) == 1 && kenv > 0 then
    printsk "pitch: %.1f, conf: %d, voiced: %d, sound: %d\n", kpitch, kconf*100, kvoiced, ksound
  endif
  ;; Switch these to compare pyin with plltrack
  aout = vco2(0.5, kpitch) * a(kenv)
  ; aout = vco2(0.5, k(apllpitch)) * a(kenv)
  aout = lpf18(aout, 2000, 0.3, 0)
  outch 1, asigpyin * 0.9
  outch 2, aout
endin

</CsInstruments>

<CsScore>
i1 1 20

</CsScore>
</CsoundSynthesizer>
