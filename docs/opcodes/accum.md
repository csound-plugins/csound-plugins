# accum

## Abstract

Simple accumulator of scalar values


## Description

`accum` can be used together with `changed`, `changed2`, `metro`, etc, 
to convert a binary trigger to an incremental one. Incremental triggers
are used by many opcodes (`printf`, for example), so by doing `accum(changed(kvar))`
it is possible to use binary triggers wherever an incremental trigger is expected.
`accum` outputs its current value and increments it afterwords. 

## Syntax


```csound
kout accum kin, initial=0
```
    
## Arguments

* `kin': the step to add. This value will be added each k-cycle
* `initial`: initial value of the accumulator

## Output

* `kout`: accumulated value

## Execution Time

* Init 

## Examples

```csound

kout accum 1, 0    ; outputs 0, 1, 2, 3, 4...

; Play a sample with variable speed, stop the event when finished
aphase accum 1
ilen = ftlen(ift)
amask 

asig table3 aphase, ift

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

* [metro](http://www.csounds.com/manual/html/metro.html)
* [changed](http://www.csounds.com/manual/html/changed.html)
* [trighold](http://www.csounds.com/manual/html/trighold.html)
* [printf](http://www.csounds.com/manual/html/printf.html)

## Credits

Eduardo Moguillansky, 2019
