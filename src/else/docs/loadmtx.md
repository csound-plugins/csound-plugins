# loadnpy

## Abstract

Load an array saved in the npy format

## Description

``loadnpy`` loads an array saved to disc in the npy format. A `.npy`
file holds a possibly multidimensional array of either int or float
type, with members of 32 or 64 bits. This opcodes converts all such
formats to a float64 array of the same shape. For more information on
the `.npy` format, see
<https://numpy.org/devdocs/reference/generated/numpy.lib.format.html>

## Syntax


```csound

iArr[] loadnpy Spath
kArr[] loadnpy Spath

```
    
## Arguments

* **Spath**: the path to the saved .npy file

## Output

* **iArr** / **kArr**: the data as an array.

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
    iA[] loadnpy "test-float64.npy"
    printarray iA

    iB[] loadnpy "test-2D.npy"
    printarray iB

    iC[] loadnpy "test-int.npy"
    printarray iC
    
    turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1
; f0 3600

</CsScore>
</CsoundSynthesizer>



```


## See also

* [hdf5read](http://www.csound.com/docs/manual/hdf5read.html)
* [ftgen](http://www.csound.com/docs/manual/ftgen.html)
* [fillarray](http://www.csound.com/docs/manual/fillarray.html)
* [ftnew](ftnew.md)
* [ftfree](http://www.csound.com/docs/manual/ftfree.html)
* [sfreadmeta](sfreadmeta.md)


## Credits

Eduardo Moguillansky, 2021
