# zeroarray

## Abstract

Zero all elements in an array


## Description

``zeroarray`` sets all elements in an array to 0. 

In the case of an audio array it is possible to pass a mask array of the same size 
as the audio array indicating which audio items in the array need to be zeroed
(only those audio items  will be zeroed for which the corresponding scalar item
in the mask array is higher than 0). The mask can also be a table

## Syntax


```csound

zeroarray iArr[]
zeroarray kArr[]
zeroarray aArr[] [, kMask[]]
zeroarray aArr[], imasktable=0

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
nchnls = 8
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

instr 30
	; test masked zeroying
	kfreqs[] fillarray 200, 300, 400, 500, 600, 700, 800, 900
	asigs[] poly 8, "oscili", 0.1, kfreqs
	; imask[] fillarray 0, 0, 1, 0, 1, 0, 0, 0
	imask ftfill 0, 0, 1, 0, 1, 0, 0, 0
	zeroarray asigs, imask
	out asigs
endin

</CsInstruments>

<CsScore>

; i 10 0 10
; i 20 0 10
i 30 0 10
</CsScore>
</CsoundSynthesizer>



```


## See also

* [ftset](https://csound.com/docs/manual/ftset.html)
* [setslice](setslice.md)
* [ftslice](https://csound.com/docs/manual/ftslice.html)


## Credits

Eduardo Moguillansky, 2021 (masked version 2022)
