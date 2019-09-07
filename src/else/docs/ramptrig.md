# ramptrig

## Abstract

A triggerable ramp between 0 and 1


## Description

`ramptrig` is a phasor between 0 and 1, with the difference that it stops after
reaching its end point. Whenever it is triggered it rewinds to 0 and starts
ramping to 1 in the given duration. 

A trigger detected whenever the value is possitive and higher than the previous value.

!!! Note "Usage as envelope generator"

    Together with `bpf` this can be used to emulate supercollider's `Env` and `EnvGen`,
    where `ramptrig` is used as a triggerable phasor, passed as an argument to `bpf`,
    which generates the envelope. See examples
    

## Syntax

    kout ramptrig ktrig, kdur, ivaluepost=1, ivaluepre=0
    kout, kfinished ramptrig ktrig, kdur, ivaluepost=1, ivaluepre=0
    
### Arguments

* `ktrig`: whenever this is possitive and higher than last value, kout is rewinded to 0
* `kdur`: the duration of the ramp
* `ivaluepost`: value when ramp reaches its end (default=1)
* `ivaluepre`: value previous to any trigger (default=0)

### Output

* `kout`: value of the ramp, between 0 and 1. It can also be `ivaluepost` or `ivaluepre` if 
          these are set to any other value than the default
* `kfinished`: will be one whenever the ramp reaches its end value. 


### Execution Time

* Performance

## Examples


```csound 

<CsoundSynthesizer>
<CsOptions>

</CsOptions>

<CsInstruments>
; This is the example file of ramptrig
; ramptrig is a triggerable ramp from 0 to 1
; xout ramptrig xtrig, kdur

sr = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

; Use Case #1: An envelope which can be retriggered
instr 1
    ; Duration of envelope
    kdur = 4
    ; This is the gate, could be any irregular signal, midi, osc, etc.    
    ktrig metro 0.5
    ; Whenever ktrig is possitive and higher than previous value, 
    ; kx ramps from 0 to 1 in kdur seconds
    kx ramptrig ktrig, kdur
    ; actual envelope
    kenv bpf kx*kdur, 0, 0, 0.02, 1, kdur, 0
    asig oscili 0.2, 1000
    ; asig pinker
    asig *= interp(kenv)
    outs asig, asig
endin

; Use Case #2: Use finished trigger to signal something
instr 2
    ktrig metro 1/4
    ktrig delayk ktrig, 0.5
    idur = 2
    kphase, kfinished1 ramptrig ktrig, 2
    printf "finished! \n", kfinished1
    kenv bpf kphase * idur, 0, 0, 0.5, 1, 0.8, 0.5, 1, 1, idur, 0
    asig = pinker() * interp(kenv)
    outs asig, asig
endin


</CsInstruments>

<CsScore>

i1 0 10
; i2 0 12
; i3 0 20
; f0 3600


</CsScore>
</CsoundSynthesizer>

```


## See also

* [linenv](linenv.md)
* [sc_phasor](https://csound.com/docs/manual/sc_phasor.html)
* [bpf](https://csound.com/docs/manual/bpf.html)

## Credits

Eduardo Moguillansky, 2019
