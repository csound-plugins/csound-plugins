# zitarev

## Abstract

Faust port of zita rev

## Description

This is a port of faust's take on zita-rev1 from
https://kokkinizita.linuxaudio.org/linuxaudio/zita-rev1-doc/quickguide.html.
This reverb is divided in 2 bands, a low band and a middle band, with a decay
time for each band. The crossover frequency is controlled by the `decaylfx` parameter.
Also an equalization curve consisting of two band eq. filters is applied. Here both
the frequencies (`eq1freq` and `eq2freq`) and levels(`eq1level` and `eq2level`) can
be set. High-frequency damping is controlled via the `hfdamp` parameter, which sets
the cutoff frequency of a low-pass shelving filter.

### Parameters

| Parameter | Description                         | Default   | Range                 |
|-----------|-------------------------------------|-----------|-----------------------|
| drywet    | Dry/Wet mix                         | 0         | -1, 1 (-1=wet, 1=dry) |
| delayms   | Pre delay                           | 60 (ms)   | 20-100                |
| decaylow  | Decay time (rt60) for low freqs.    | 3 (secs)  | 0.01 - 10             |
| decaymid  | Decay time (rt60) for middle freqs. | 2 (secs)  | 0.01 - 10             |
| hfdamp    | High-frequency damping              | 6000 (hz) | 1500-20000            |
| decaylfx  | Decay crossover freq.               | 200 (hz)  | 50-1000               |
| eq1level  | Level for band eq filter 1          | 0 (dB)    | -15, 15               |
| eq1freq   | Frequency for eq filter 1           | 315 (hz)  | 100-20000             |
| eq2level  | Level for band eq filter 2          | 0 (dB)    | -15, 15               |
| eq2freq   | Frequency for eq filter 2           | 1500      | 100-20000             |
| level     | Gain level                          | -20 (dB)  | -70-40                |


## Syntax


```csound
aout1, aout2 zitarev ain1, ain2 [, Sparam1, kvalue1, ...]
```

## Arguments

* **ain1**: left input channel
* **ain2**: irght input channel
* **Sparam_n**: name of a control (see below)
* **kvalue_n**: value for the given control



## Output

* **aout1**: left output channel
* **aout2**: right output channel


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


instr 1
  ipitch = p4
  iamp = db(p5)
  ipan = p6
  prints "note %f\n", ipitch
  ifreq = mtof:i(ipitch)
  asig = vco2:a(iamp, ifreq)
  asig = lpf18(asig, ifreq * 8, 0.85, 0) * 4
  asig *= adsr(0.005, 0.05, 0.2, p3*0.5) 
  aL, aR pan2 asig, ipan
  chnmix aL, "rev1"
  chnmix aR, "rev2"
endin

instr 2
  ipitches[] genarray 48, 72, 0.25
  i0 = 0
  iN = lenarray(ipitches)
  while i0 < iN do
    ipan = (i0 / iN * 4) % 1
	schedule 1, i0*0.4, 0.2, ipitches[i0], -3, ipan
	i0 += 1
  od
endin

instr 10
  aL chnget "rev1"
  aR chnget "rev2"
  arev1, arev2 zitarev aL, aR, "drywet", -0.25, "level", -3, "delayms", 30, "decaylow", 4, "decaymid", 2.5, "decaylfx", 300, "eq1level", 4, "eq2level", 3
  outch 1, arev1, 2, arev2
  chnclear "rev1", "rev2"
endin

</CsInstruments>

<CsScore>
i 2  0 0
i 10 0 60

</CsScore>
</CsoundSynthesizer>



```

## See also

* [reverbsc](http://www.csound.com/docs/manual/reverbsc.html)

## Credits

Eduardo Moguillansky, 2024
