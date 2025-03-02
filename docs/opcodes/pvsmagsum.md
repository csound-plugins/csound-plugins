# pvsmagsum

## Abstract

Sum all magnitudes of the bins in a pv signal

## Description

Given an pv chain, sum all magnitudes of the bins between minfreq
and maxfreq.

Used together with pvstrace it can determine how concentrated
the energy of a signal is, giving an idea of how "voiced" or "pitched"
it is. A noisy signal will tend to have energy spread more evenly across
the spectrum, while a musical or speech signal will tend to concentrate
that signal along a limited range of beans. By selecting the loudest
bins via pvstrace and comparing the energy of the selected pv signal
to the original signal this renders a quite relyable measurement of
the "peakyness" of the signal.

!!! note

    See [pvsmagsumn](pvsmagsumn.md) for a more efficient implementation,
    combining pvsmagsum and pvstrace.

## Syntax


```csound
ktotalmag pvsmagsum fsig, kminfreq=0, kmaxfreq=sr/2
```

## Arguments

* **fsig**: Input pv stream.
* **kminfreq**: min. freq to considere
* **kmaxfreq**: max. freq to considere

## Output

* **ktotalmag**: The sum of the magnitude (amp) of the bins between minfreq and maxfreq

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
  fsig2 = pvstrace(fsig, 10)
  kmagsum0 = pvsmagsum(fsig, 50)
  kmagsum = pvsmagsum(fsig2, 50)
  kpeakyness = kmagsum0 == 0 ? 0 : kmagsum / kmagsum0
  kpeakyness = lag(kpeakyness, 0.2)
  if metro(30) == 1 then
    Speak strmul ":", int(kpeakyness*40)
    printsk "Source: %d, %s, peakyness: %.3f %s\n", ksource, Snames[ksource], kpeakyness, Speak
  endif
  outch 1, asig
endin

</CsInstruments>

<CsScore>
i1 0 20

</CsScore>
</CsoundSynthesizer>



```


## See also

* [pvsmagsumn](pvsmagsumn.md)
* [pvsflatnesscent](pvsflatness.md)
* [pvscrest](pvscrest.md)

## Metadata

* Author: Eduardo Moguillansky
* Year: 2025
* Plugin: else
* Source: https://github.com/csound-plugins/csound-plugins/blob/master/src/else/src/else.c
