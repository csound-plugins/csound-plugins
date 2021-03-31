# zeroarray

## Abstract

Zero all elements in an array


## Description

``zeroarray`` sets all elements in an array to 0. 

## Syntax


```csound

zeroarray iArr
zeroarray kArr
zeroarray aArr

```
    
## Arguments

* **iArr** / **kArr** / **aArr**: The array to zero

## Output


## Execution Time

* Init 
* Performance

## Examples


```csound


<CsoundSynthesizer>
<CsOptions>
; -odac
</CsOptions>

<CsInstruments>

/*

Description
===========

zeroarr zeroes all elements in an array of any (numeric) kind

Syntax
======

    zeroarr karr
    zeroarr arr

*/

ksmps = 32
nchnls = 2
0dbfs  = 1

gabuses[] init 4

instr 10
    asig vco2 0.1, 1000
    gabuses[0] = gabuses[0] + asig
endin

instr 20
    asig = gabuses[0]
    outch 1, asig
    zeroarray gabuses
endin


</CsInstruments>

<CsScore>

i 10 0 10
i 20 0 10

</CsScore>
</CsoundSynthesizer>



```


## See also

* [ftset](https://csound.com/docs/manual/ftset.html)
* [setslice](setslice.md)
* [ftslice](https://csound.com/docs/manual/ftslice.html)


## Credits

Eduardo Moguillansky, 2021
