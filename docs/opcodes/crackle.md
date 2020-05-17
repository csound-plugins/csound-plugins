# crackle

## Abstract

generates noise based on a chaotic equation

## Description

`crackle` is a chaotic generator, the sound is generated with 
the following equation

    y[n] = p * y[n-1]- y[n-2] - 0.05

Port of supercollider's [Crackle](http://doc.sccode.org/Classes/Crackle.html)

## Syntax

```csound

aout crackle kp
    
```
    
### Arguments

* `kp`: the p parameter in the equation, a value between 1.0 and 2.0

### Output

* `aout`: audio output of the chaotic generator

### Execution Time

* Performance (audio)

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

/*

  # This is the example file for the crackle opcode

  crackle is a port of supercollider's crackle (taken from pd/else)

  crackle is a chaotic noise generator based on the equation:

  y[n] = p * y[n-1] - y[n-2] - 0.05
  
  ## Syntax

    aout crackle kp=0.5

  * kp: value for p in the equation (default = 0.5)
  * aout: noise signal

*/

sr = 44100
ksmps = 64
0dbfs = 1
nchnls = 2


instr 1
  kP[] fillarray 1.0, 1.2, 1.3, 1.35, 1.4, 1.5, 1.8, 1.9, 1.97, 2.0
  kidx = int(line(0, p3, lenarray(kP)))
  kp = kP[kidx]
  printk2 kp
  aout crackle kp
  aout *= 0.5
  outs aout, aout
  
endin


</CsInstruments>
<CsScore>
i 1 0 15

</CsScore>
</CsoundSynthesizer>


```


## See also

* [standardchaos](standardchaos.md)
* [chuap](https://csound.com/docs/manual/chuap.html)
* [dust2](https://csound.com/docs/manual/dust2.html)

## Credits

Eduardo Moguillansky, 2019
