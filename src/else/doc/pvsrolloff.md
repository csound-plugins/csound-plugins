# pvsrolloff

## Abstract

Spectral rolloff point

## Description

Spectral rolloff represents the frequency below which a specific percentage (usually 85% or 95%) of an 
audio signal's total spectral energy resides. It measures the "brightness" or skewness of a sound, 
with higher values indicating more high-frequency content.

## Syntax


```csound
krolloff pvsrolloff fsig, kfactor, kminfreq=0, kmaxfreq=sr/2, ksilenceamp=0.000001
```

## Arguments

* **fsig**: Input pv stream.
* **kfactor**: the rolloff factor, a value between 0-1. A rolloff of 85% would be indicated as 0.85
* **kminfreq**: the min. frequency to calculate the rolloff over (defaults to 0)
* **kmaxfreq**: the max. frequency to calculate the rolloff over (defaults to sr/2)
* **silenceamp**: any frame of audio with a total energy of less than this is considered silence,
  and will return a rolloff frequency of 0.

## Output

* **krolloff**: spectral rolloff frequency

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

/* Example file for pvsmagsum

krolloff pvsrolloff fsig, kfactor, kminfreq=0, kmaxfreq=sr/2

  Rolloff frequency at which the ratio of the energy below it equals the given factor

  A returned freq. of 1000 Hz at a factor of 0.9 would indicate that 90% of 
  all energy is concentrated between 0 and 1000 Hz

*/

instr 1
  asig1 = oscili:a(0.5, 500)
  asig2 = buzz(0.1, 300, 7, -1)
  asig3 = pinker() * 0.1
  asig4 = unirand:a(2) - 1.0
  asig5 = diskin2("finnegan01.flac", 1, 0, 1)[0]
  asig6 = 0
  Snames[] fillarray "sine", "buzz", "pink", "white", "finn", "silence" 
  ksource init 0
  if metro(0.5) == 1 then
    ksource = (ksource + 1) % 6
  endif
  ifftsize = 2048
  asig = picksource(ksource, asig1, asig2, asig3, asig4, asig5, asig6)
  fsig = pvsanal(asig, ifftsize, 256, ifftsize, 0)
  ifactor = 0.8
  krolloff = pvsrolloff(fsig, ifactor, 50, 20000)
  if metro(40) == 1 then
    printsk "Source: %d, %s, rolloff (%.2f): %d Hz\n", ksource, Snames[ksource], ifactor, krolloff
  endif
  outch 1, asig
endin

</CsInstruments>

<CsScore>
i1 0 30

</CsScore>
</CsoundSynthesizer>



```


## See also

* [pvsmagsumn](pvsmagsumn.md)
* [pvsflatness](pvsflatness.md)
* [pvscrest](pvscrest.md)
* [pvsentropy](pvsentropy.md)


## Metadata

* Author: Eduardo Moguillansky
* Year: 2026
* Plugin: else
* Source: https://github.com/csound-plugins/csound-plugins/blob/master/src/else/src/else.c
