# pvscrest

## Abstract


Compute the spectral crest value of a spectral signal.

## Description

Given an pv chain, this produces the spectral crest measure, which is an 
indicator of the "peakiness" of the spectral energy distribution. For 
example, white noise should produce a flat (non-peaky) spectrum, and 
therefore a low value for the spectral crest.

Optionally, kminfreq and kmaxfreq indicate the lower and upper frequency 
limits of the band to look at; by default, the whole FFT range (excluding 
DC and nyquist) is analysed.

In pseudo-equation form, the measure is calculated as follows:

Crest = S.maxItem / S.mean

where "S" is a list of the squared magnitudes in the spectral band. Note that 
this limits the value to being greater than or equal to 1. (Some research uses 
a logarithmic scale - you can apply the logarithm yourself if required.)

pvscrest is a port of supercollider's FFTCrest ugen

## Syntax


```csound
kcrest pvscrest fsig, kminfreq=0, kmaxfreq=sr/2
```

## Arguments

* **fsig**: Input pv stream.

## Output

* **kflatness**: spectral flatness of the signal
* **kminfreq**: min. frequency bin used for calculation
* **kmaxfreq**: max. frequency bin used for calculation

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

/* Example file for pvscrest

kvalue pvscrest kminfreq=0, kmaxfreq=sr/2

  Given an pv chain, this produces the spectral crest measure, which is an 
  indicator of the "peakiness" of the spectral energy distribution. For 
  example, white noise should produce a flat (non-peaky) spectrum, and 
  therefore a low value for the spectral crest.
  
  Optionally, kminfreq and kmaxfreq indicate the lower and upper frequency 
  limits of the band to look at; by default, the whole FFT range (excluding 
  DC and nyquist) is analysed.
  
  In pseudo-equation form, the measure is calculated as follows:
  
  Crest = S.maxItem / S.mean
  
  where "S" is a list of the squared magnitudes in the spectral band. Note that 
  this limits the value to being greater than or equal to 1. (Some research uses 
  a logarithmic scale - you can apply the logarithm yourself if required.)

  pvscrest is a port of supercollider's FFTCrest ugen
*/

instr 1
  asig1 = oscili:a(0.5, 500)
  asig2 = buzz(0.1, 300, 7, -1)
  asig3 = pinker() * 0.1
  asig4 = unirand:a(2) - 1.0
  Snames[] fillarray "sine ", "buzz ", "pink ", "white"
  ksource init 0
  if metro(0.5) == 1 then
    ksource = (ksource + 1) % 4
  endif
  asig = picksource(ksource, asig1, asig2, asig3, asig4)
  fsig = pvsanal(asig, 512, 256, 512, 0)
  kcrest = pvscrest(fsig, 100)
  kcrest = lag(kcrest, 0.2)
  if metro(20) == 1 then
    printsk "Source index: %d, signal: %s, crest: %.3f\n", ksource, Snames[ksource], kcrest
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
* [pvsflatnesst](pvsflatness.md)
* [pvsmagsum](pvsmagsum.md)


## Metadata

* Author: Eduardo Moguillansky
* Year: 2025
* Plugin: else
* Source: https://github.com/csound-plugins/csound-plugins/blob/master/src/else/src/else.c
