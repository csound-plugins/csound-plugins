# pyin

## Abstract

Fundamental tracking implementing the pYIN algorithm

## Description

pYIN is a modificatin of the YIN algorithm for f0 estimation. In the first step 
of pYIN, F0 candidates and their probabilities are computed using the YIN algorithm. 
In the second step, Viterbi decoding is used to estimate the most likely F0 
sequence and voicing flags. This streaming version uses only past observations 
(no lookahead) and is thus somewhat less effective than offline pyin.  
  
## Syntax

```csound
kfreq, kconfidence, kvoiced pyin asig, Sarg1, ivalue1, [Sarg2, ivalue2, ...]
```

## Arguments

* **asig**: audio signal
* **Sarg1**: argument name, see below
* **ivalue1**: argument value

### Named arguments:

* **framesize**: 1024–8192 (2048). Analysis frame size in samples
* **hop**: 64– (1/4 framesize). Hop size in samples
* **fmin**: 20– (60). Min. frequency
* **fmax**: 20- (1000). Max. frequency
* **bins**: 2–50 (10). Subdivisions per semitone
* **transition_weight**: 0–1 (0.1). Prob. of switching voiced↔unvoiced
* **minrms**: 0–1. Mark sound with rms below this as silent
* **beta_a**: 1–3. Param a for beta distr. prior over thresholds
* **beta_b**: 1–20. Param b for beta distr. Lower = smoother track
* **drift**: 10– (100). Pitch variation between frames, in cents
* **voiced_obs_floor**: 0–1. Floor for voiced observations
* **octave_cost**: 0–0.5 (0). Cost of jumping an octave between frames. 
  Use if the algorithm falsely predicts the 2nd overtone as the fundamental
* **subharmonic_tresh**: 4. Used together with octave_cost, controls the 
  threshold of a downward octave jump


## Output

* **kfreq**: detected frequency. Only valid if confidence is > ~0.4
* **kconfidence**: detection confidence
* **kvoiced**: is the sound voiced


## Execution Time

* Performance

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
  	"drift", 120, "voiced_obs_floor", 0.1
  ; akOOOOO
  apllpitch, aloc plltrack asigpyin, 0.05, 15, 0.15, 70, 600, 0.0001
  ksound = schmitt(dbamp(rms(asig)),  -45, -60);
  kvalid = schmitt:k(kconf, 0.4, 0.3) * schmitt:k(kpitch, 90, 75) * (1-schmitt:k(kpitch, 400, 300))
  kenv = kvalid * ksound;
  if metro(48) == 1 && kenv > 0 then
    printsk "pitch: %.1f, conf: %d, voiced: %d, sound: %d\n", kpitch, kconf*100, kvoiced, ksound
  endif
  ;; Switch these to compare pyin with plltrack
  aout = vco2(0.5, kpitch) * a(kenv)
  aenv = lagud(a(kenv), 0.03, 0.15)
  ; aout = vco2(0.5, k(apllpitch)) * aenv
  aout = lpf18(aout, 1500, 0.4, 0)
  outch 1, asigpyin * 0.9
  outch 2, aout
endin

</CsInstruments>

<CsScore>
i1 1 20

</CsScore>
</CsoundSynthesizer>



```


## See also

* [pyinf0](pyin.md)
* [ptrack](https://csound.com/docs/manual/ptrack.html)
* [plltrack](https://csound.com/docs/manual/plltrack.html)
* [pvsmagsum](pvsmagsum.md)
* [pvsflatness](pvsflatness.md)

## Metadata

* Author: Eduardo Moguillansky
* Year: 2026
* Plugin: pitchtrack
* Source: https://github.com/csound-plugins/csound-plugins/blob/master/src/pitchtrack/src/pyin.c
