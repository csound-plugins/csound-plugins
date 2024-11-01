# transpose

## Abstract

Simple delay based pitch shifter inspired on faust's transpose

## Description

A simple pitch shifter based on 2 delay lines

## Syntax


```csound

aout transpose ain, ksemitones, kwindur=0.02, kxfade=-1

```
    
## Arguments

* **ain**: The audio signal
* **ksemitones**: The semitones to shift the signal, up or down
* **kwindur**: The duration of the window used, in seconds. The lower the 
  sound to be transposed, the longer the duration of the window.
* **kxfade**: The size of the crossfade, in samples. Defaults to 1/4 of the 
  window duration, in samples. This parameter is quite sensitive to the kind
  of sound used (sustained vs. percussive, voiced vs. unvoiced) so some
  adaptation to each case might be needed.

## Output

* **aout**: The transposed signal

## Execution Time

* Performance

## Examples


```csound


<CsoundSynthesizer>
<CsOptions>

</CsOptions>

<CsInstruments>
sr     = 48000
ksmps  = 64
nchnls = 2
0dbfs  = 1


opcode nativetranspose, a, akkp
  ; native implementation by J. Heintz
  asig, ksemitones, kwindur, iwindow xin
  imaxdelay = 2
  kfreqratio = semitone(ksemitones)
  iwindowtab = ftgenonce(0, 0, 4096, 20, iwindow, 1)
  kphasorfreq = (1 - kfreqratio) / kwindur
  aphasor1 = phasor:a(kphasorfreq)
  aphasor2 = phasor:a(kphasorfreq, 0.5)
  adelay1 = vdelayx(asig, aphasor1 * kwindur, imaxdelay, 4)
  adelay2 = vdelayx(asig, aphasor2 * kwindur, imaxdelay, 4)
  adelay1 *= tablei:a(aphasor1, iwindowtab, 1)
  adelay2 *= tablei:a(aphasor2, iwindowtab, 1)
  adelay1 += adelay2
  xout adelay1
endop

instr 1
  kwindur = 0.02
  asig = vco2(0.5, mtof:i(ntom("2G")))
  ; asig = oscili:a(0.8, mtof:i(ipitch))
  asig = gtadsr(asig, 0.004, 0.008, 0.2, 0.3, metro(7/3))
  ktime = eventtime()
  kshift = bpf:k(ktime, 0, 0, 8, 12, 12, 12, 20, 0)
  kxfade = kwindur * sr / 2.5
  ashifted = transpose(asig, kshift, kwindur, kxfade)
  ashifted2 = nativetranspose(asig, kshift, kwindur, 1)
  outs ashifted, ashifted2
endin

</CsInstruments>

<CsScore>
i1 0 25

</CsScore>
</CsoundSynthesizer>



```


## See also

* [pvshift](https://csound.com/docs/manual/pvshift.html)
* [trshift](https://csound.com/docs/manual/trshift.html))


## Credits

Eduardo Moguillansky, 2023


## Metadata

* Author: Eduardo Moguillansky
* Year: 2023
* Plugin: else
* Source: https://github.com/csound-plugins/csound-plugins/blob/master/src/else/src/else.c
