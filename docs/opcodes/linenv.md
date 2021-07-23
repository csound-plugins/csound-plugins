# linenv

## Abstract

A triggerable linear envelope with sustain segment 


## Description

`linenv` is similar to linsegr with an additional gate argument, allowing
to retrigger it at will. One of the values can be defined as a sustain point,
meaning that as long as the gate is held, the envelope enters a sustain
state when reaching this point. When the gate is set to 0, the envelope traverses
the rest of the defined points.

If the gate is closed before reaching the sustain segment, the envelope slides to
the release section skipping any segments in between.

!!! warning

    Unlike `linsegr`, `linenv` does not extend the time of the instrument. Use
    `xtratim` for that
    
!!! note

    Similarly to linseg/linsegr, it is possible to define segments of 0 duration to
    force sharp jumps

## Syntax

```csound

xout linenv kgate, isustindex, kval0, [ktime1, kval1, ktime2, kval2, ...]

```

### Arguments

* `kgate`: whenever this switches from 0 to 1 a new envelope starts. If a sustain segment
  was defined, when closing the gate (transition from 1 to 0) the envelope jumps to the sustain
  segment. Otherwise it just continues until it reaches the last segment (a "oneshot" envelope)
* `isustindex`: the index of the sustain point. For example, if `isustindex` is 2, then 
when the envelope reaches kval2 (after ktime0+ktime1), it enters a sustain phase, where
its value remains unmodified until the gate is set to 0. If no sustain point is desired,
set `isustindex` to 0. **NB**: negative values count from end. To imitate the behaviour of
linsegr, use -1 as sustain index.
* `kval0`, `ktime1`, ...: a linear envelope, similar to `linsegr`. The release part can have
as many segments as desired. **ktime<sub>n</sub>** values are defined as time interval between two values,
**not as absolute timestamps**

### Output

* `xout`: value of the envelope (k- or a- rate)

### Execution Time

* Performance

## Examples

```csound


<CsoundSynthesizer>

<CsOptions>
-odac
</CsOptions>

<CsInstruments>

/*
    This is the example file for opcode "linenv"
    
    linenv is a triggerable envelope with a sustain segment

    aout linenv kgate, isustidx, kval0, ktime1, kval1, ..., ktimen, kvaln

    NB: use xtratim if necessary to allow for release segment 
    
*/


sr = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1
gkgate init 0

FLpanel "linenv", 240, 100, 100, 100
	gkgate, gih1 FLbutton " Gate", 1, 0, 2, 80, 40, 10, 10, -1  
FLpanelEnd
FLrun

instr 1
    kgate = trighold:k(metro(1/2), 0.5)
    kenv linenv kgate, 1, 0, 0.15, 1, 0.1, 0
    printf "t: %f,  kenv: %f \n", timeinstk(), timeinsts(), kenv

    kenv *= 0.2
    asig = pinker() * interp(kenv)
    outs asig, asig
endin

instr 2
    iperiod = 2
    igatedur = 1
    kgate = trighold:k(metro(1/iperiod), igatedur)
    aenv linenv kgate, -2, 0, 0.05, 1, 0.2, 0.5, 0.2, 1, 0.4, 0
    asig = oscili:a(0.2, 1000) * aenv
    FLsetVal changed(kgate), kgate, gih1
    outs asig, asig
endin

instr 3
	asig pinker
	aenv linenv gkgate, 2, 0, 0.05, 1, 0.1, 0.2, 0.5, 0
	asig *= aenv
	outs asig, asig
endin

instr 4
    ; no sustain ("one shot")
    asig pinker
	aenv linenv gkgate, 0, 0, 0.05, 1, 0.1, 0.2, 0.5, 0
	asig *= aenv
	outs asig, asig
endin
    
</CsInstruments>

<CsScore>

; i1 0 10
; i2 0 10
i3 0 100
; i4 0 100
</CsScore>
</CsoundSynthesizer>


```


## See also

* [ramptrig](ramptrig.md)
* [bisect](bisect.md)
* [interp1d](interp1d.md)
* [bpf](https://csound.com/docs/manual/bpf.html)
* [linsegr](https://csound.com/docs/manual/linsegr.html)

## Credits

Eduardo Moguillansky, 2019

(idea based on pd/else's `envgen` and supercollider's `envgen`)
