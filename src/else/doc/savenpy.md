# savenpy

## Abstract

Save a table or array to a .npy file

## Description

``savenpy`` saves an array or table as a numpy .npy file. A .npy file holds 
a possibly multidimensional array of either int or float type, with members 
of 32 or 64 bits. For more information on the `.npy` format, see
<https://numpy.org/devdocs/reference/generated/numpy.lib.format.html>

## Syntax


```csound

savenpy Soutfile, itabnum
savenpy Soutfile, iarr[]
savenüy Soutfile, karr[]

```
    
## Arguments

* **Soutfile**: the path to a .npy file where data will be written
* **tabnum**: the table number of the source table
* **iarr**: an array to save (can be any dimension)
* **karr**: an array to save (can be any dimension)

## Execution Time

* Init 

## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
	itab ftfill 1, 10, 20, 20, 3, 30, 0.5, 5
	savenpy "test.npy", itab
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

* [loadnpy](loadnpy.md)
* [ftfill](ftfill.md)
* [foutk](http://www.csound.com/docs/manual/foutk.html)

## Credits

Eduardo Moguillansky, 2026
