# <opcodename>

## Abstract

<One-line description of this opcode>


## Description

<Long description of the opcode>


## Syntax


```csound
kout accum kstep, initial=0, kreset=0
aout accum kstep, initial=0, kreset=0

```
    
## Arguments

* **kstep**: the step to add. This value will be added at each iteration (at each k-cycle 
    for `accum:k` and at each sample for `accum:a`)
* **initial**: initial value of the accumulator
* **kreset**: if 1, the accummulator is reset to the initial value

## Output

* **kout**: accumulated value

## Execution Time

<Either 'Init', 'Performance' or both>

* Performance 


## Examples


```csound

kout accum 1, 0    ; outputs 0, 1, 2, 3, 4...

; Play a sample with variable speed, stop the event when finished
aindex accum 1
kspeed = linseg:k(0.5, ilen, 2)
ilen = ftlen(ift)
aindex *= kspeed
asig table3 aindex, ift
if aindex[0] >= ilen - (ksmps*kspeed) then
    turnoff
endif
ifade = 1/ksmps
out asig * linsegr(0, ifade, 1, ifade, 0)

```

```csound


<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

instr 1
  kx linseg 0, p3, 1
  printf "kx=%f \n", accum(changed(kx)), kx
  ; the same without accum would only print the first time,
  ; since changed would return always 1 but printf expects an ever
  ; increasing trigger
endin

</CsInstruments>
<CsScore>

i1 0 0.1

</CsScore>
</CsoundSynthesizer>


```


## See also

* [metro](http://www.csound.com/docs/manual/metro.html)
* [changed](http://www.csound.com/docs/manual/changed.html)
* [trighold](http://www.csound.com/docs/manual/trighold.html)
* [printf](http://www.csound.com/docs/manual/printf.html)

## Credits

<Name>, <Year>
