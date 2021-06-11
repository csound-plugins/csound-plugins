# detectsilence

## Abstract

Detect when input falls below an amplitude threshold

## Description

When the absolute value of the input signal remains below the
threshold for a given window of time, output 1. Otherwise, output 0.
This can be used to detect the end of a sample or the end of a complex
envelope

`detectsilence` is a port of SuperCollider's `DetectSilence` UGen
(<https://doc.sccode.org/Classes/DetectSilence.html>)

## Syntax

```csound

kout detectsilence asig, kthresh=0.0001, ktime=0.1

```

## Arguments

* **asig**: the audio signal to analyze
* **kthresh**: the amplitude threshold
* **ktime**: the time period the signal should stay below the threshold

## Output

* **kout**: 1 if the signal has been below the threshold for the given time, 0 otherwise


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

instr 1
  asig oscili 0.5, 1000
  aenv linseg 0, 0.1, 1, 1.9, 1, 1, 0 ; total 0.1+1.9+1=3
  asig *= aenv
  kfinished = detectsilence(asig, db(-90), 0.1)
  kms = timeinsts() * 1000
  if metro(30) == 1 then
    printsk "\r Elapsed time: %.2f ms, env: %.5f         ", kms, aenv[0]
  endif
  if kfinished == 1 then
    turnoff
  endif
  defer "prints", "\nInstrument exited after %.2f ms\n", kms 
endin

</CsInstruments>

<CsScore>

i1 1 8
; f0 3600

</CsScore>
</CsoundSynthesizer>



```


## See also

* [follow2](http://www.csound.com/docs/manual/follow2.html)
* [rms](http://www.csound.com/docs/manual/rms.html)


## Credits

Eduardo Moguillansky, 2021
