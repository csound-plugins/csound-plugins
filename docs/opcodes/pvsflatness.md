# pvsflatness

## Abstract

Compute the flatness of a spectral signal.

## Description

Given a pvs chain this calculates the Spectral Flatness measure, defined as a
power spectrum's geometric mean divided by its arithmetic mean. This gives a
measure which ranges from approx 0 for a pure sinusoid, to approx 1 for white noise.

The measure is calculated linearly. For some applications you may wish to convert
the value to a decibel scale

pvsflatness is a port of supercollider's SpecFlatness ugen

## Syntax


```csound
kflatness pvsflatness fsig
```

## Arguments

* **fsig**: Input pv stream.

## Output

* **kflatness**: spectral flatness of the signal

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

/* Example file for pvsflatness

kflatness pvsflatness fsig

  Given a pvs chain this calculates the Spectral Flatness measure, defined as a
  power spectrum's geometric mean divided by its arithmetic mean. This gives a
  measure which ranges from approx 0 for a pure sinusoid, to approx 1 for white noise.

  The measure is calculated linearly. For some applications you may wish to convert
  the value to a decibel scale

  pvsflatness is a port of supercollider's SpecFlatness ugen
*/

instr 1
  asig1 = oscili:a(0.1, 500)
  asig2 = buzz(0.1, 300, 7, -1)
  asig3 = pinker() * 0.1
  asig4 = unirand:a(2) - 1.0
  Snames[] fillarray "sine ", "buzz ", "pink ", "white"
  ksource init 0
  if metro(0.5) == 1 then
    ksource = (ksource + 1) % 4
  endif
  asig = picksource(ksource, asig1, asig2, asig3, asig4)
  fsig = pvsanal(asig, 1024, 512, 1024, 0)
  kflatness = pvsflatness(fsig)
  kflatness = lag(kflatness, 0.3)
  if metro(12) == 1 then
    printsk "Source index: %d, signal: %s, Flatness: %f, (%05.1f dB)           \n", ksource, Snames[ksource], kflatness, dbamp(kflatness)
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

* [pvscent](http://www.csound.com/docs/manual/pvscent.html)

## Metadata

* Author: Eduardo Moguillansky
* Year: 2025
* Plugin: else
* Source: https://github.com/csound-plugins/csound-plugins/blob/master/src/else/src/else.c
