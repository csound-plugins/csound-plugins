# fofcyclevoc

## Abstract

Port of faust's "Sf Formant FOF Cycle" vocal model

## Description

Simple formant/vocal synthesizer based on a source/filter model. The source
is just a periodic impulse and the "filter" is a bank of FOF filters. Formant
parameters are linearly interpolated allowing to go smoothly from one vowel to
another. Voice type can be selected but must correspond to the frequency range
of the synthesized voice to be realistic. This model does not work with noise
in the source signal so exType has been removed and model does not depend on
SFFormantModel function. Source: https://faustlibraries.grame.fr/libs/physmodels/#pmsfformantmodelfofcycle


### Keyword Parameters

| Parameter | Description                                                          | Default  | Range (not enforced) |
|-----------|----------------------------------------------------------------------|----------|----------------------|
| vibfreq   | Vibrato frequency                                                    | 6        | 4 - 8                |
| vibgain   | Vibrato gain (amplitude of vib LFO)                                  | 0.5      | 0-1                  |
| vowel     | Vowel (0=a, 1=e, 2=i, 3=o, 4=u)                                      | 0        | 0-4                  |
| voicetype | Voice type (0: alto, 1: bass, 2: countertenor, 3: soprano, 4: tenor) | 0        | 0-4                  |
| envattack | Attack time (in milliseconds)                                        | 10       | 0-500                |
| bend      | Bend in semitones applied to the base frequency.                     | 0        | -2 - 2               |
| sustain   | Amplitude when gate is off                                           | 0        | 0-1                  |
| outgain   | Gain factor applied at the output                                    | 0.5      | 0-1                  |


## Syntax

```csound
aout fofcyclevoc kgate, kfreq, kgain [, Sparam_n, kvalue_n, ...]
```

Multiple pairs of Skey: kvalue can be given (see the table above for possible keys)

## Arguments

* **kgate**: Gate. A new "note" starts when the gate is open and stops when the gate is close
* **kfreq**: Fundamental frequency
* **kgain**: Excitation gain
* **Sparam_n**: Parameter name to modify (constant string).
* **kvalue_n**: Parameter value corresponding to the previous name


## Output

* **aout**: Output signal


## Execution Time

* Performance

## Examples

```csound


<CsoundSynthesizer>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
  kgate = trighold:k(metro(4), 0.5)
  kpitch = 60
  asig fofcycle kgate, mtof:k(kpitch), 0.9, "vowel", 0
  outch 1, asig
endin  

</CsInstruments>

<CsScore>
i 1 0 10


</CsScore>
</CsoundSynthesizer>



```

## See also

* [fof2](http://www.csound.com/docs/manual/fof2.html)

## Credits

Eduardo Moguillansky, 2024
