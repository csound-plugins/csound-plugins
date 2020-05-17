# frac2int

## Abstract

Convert the fractional part of a number into an integer


## Description

`frac2int` can be used to convert the fractional part of a number (as passed,
for example, as `p1`) back to an integer.

## Syntax

```csound

iInt frac2int iFloat, iMul
kInt frac2int kFloat, kMul

```

## Arguments

* `iFloat` / `kFloat`: a number with a fractional part (for example, a
  fractional `p1`)
* `iMul` / `kMul`: the factor used to convert the initial integer into a
  fraction (see example)

## Output

* `iInt` / `kInt`: the integer value corresponding to the fractional part

!!! Note

    The integral part of the passed value is discarded


```csound

ival = 10 + 123 / 1000     ; => 10.123
ival2 frac2int ival, 1000  ; => 123
```

## Execution Time

* Init
* Performance

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

instr 1
  inum frac2int 1.45, 100
  print inum
  turnoff
endin

</CsInstruments>

<CsScore>

i1 0 0.1
; f0 3600

</CsScore>
</CsoundSynthesizer>

```


## See also

* [frac](http://www.csounds.com/manual/html/frac.html)

## Credits

Eduardo Moguillansky, 2019
