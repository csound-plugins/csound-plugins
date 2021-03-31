# ftfind

## Abstract

Find an element in a table

## Description

``ftfind`` finds the index of an element in table. If the element is not found
it returns -1

## Syntax


```csound

iindex ftfind itab, ival, itolerance=1e-12
kindex ftfind ktab, kval, itolerance=1e-12


```
    
## Arguments

* **itab** / **ktab**: A table
* **ival** / **kval**: The value to find
* **itolerance**: A tolerance value. When using floats it is not recommended to 
check for equality but to check if two values are close enough

## Output

* **iindex** / **kindex**: the index of the value inside the table, or -1 if the value is not found

## Execution Time

* Init 
* Performance

## Examples


```csound


<CsoundSynthesizer>
<CsOptions>
--nosound
; -odac
</CsOptions>

<CsInstruments>

/*

Description
===========

ftfind return the index of the first element in a table which is equal 
to the given number. If the number is not found, the return value is -1

Syntax
======

    kidx ftfind ktabnum, kvalue, iepsilon=1e-12
    iidx ftfind itabnum, ivalue, iepsilon=1e-12
    
*/

ksmps = 64
nchnls = 2
0dbfs  = 1

instr 10
    itabnum ftfill 0, 0.5, 0.3, 10, 0.8
    iidx ftfind itabnum, 0.3
    prints "iidx: %d \n", iidx

    turnoff
endin

</CsInstruments>

<CsScore>

i 10 0 0.1

</CsScore>
</CsoundSynthesizer>



```


## See also

* [findarray](findarray.md)

## Credits

Eduardo Moguillansky, 2021
