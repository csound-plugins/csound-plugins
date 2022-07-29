# mtro

## Abstract

an accurate version of metro


## Description

`mtro` is a drop-in replacement of `metro` without the
drifting over time. `metro` is implemented by adding a phase
each cycle, which results in drifting over time due to rounding errors.
`mtro` avoids this problem by keeping a counter and doing multiplication
so that the error is limited.

Following metro's implementation by default `mtro` is triggered at time=0. This
is because `i_initphase` is set to 1 by default. It is possible to make `mtro` skip the
0 trigger by setting `i_initphase` to 0.

## Syntax


```csound

kout mtro kfreq, i_initphase=1

```
    
## Arguments

* **kfreq**: the frequency of the trigger
* **i_initphase**: the initial phase (a value between 0 and 1). If set to 0, there is no trigger at time 0.
	If set to 1, `mtro` triggers at time=0, similar to `metro`

## Output

* **kout**: 1 if the mtro has triggered, 0 otherwise

## Execution Time

* Performance

## Examples


```csound


<CsoundSynthesizer>
<CsOptions>
-odac 
--nosound
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 64


opcode gettime, k, 0
  xout (timeinstk:k() - 1) * (ksmps/sr)
endop

instr 1
  ktrig mtro 7
  kt = gettime()
  if ktrig == 1 then
    println "triggered! time: %f", kt
  endif
endin

instr 2
  ktrig metro 7
  kt = gettime()
  if ktrig == 1 then
    println "triggered! time: %f", kt
  endif
endin


</CsInstruments>
<CsScore>
i 1 0 20
; i 2 0 20

</CsScore>
</CsoundSynthesizer>



```


## See also

* [metro](https://csound.com/docs/manual/metro.html)


## Credits

Eduardo Moguillansky, 2021
