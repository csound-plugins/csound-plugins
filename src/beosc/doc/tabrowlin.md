# tabrowlin

## Abstract

Copy a row from an f-table to another, interpolating between rows


## Description

This opcode assumes the use of a table, which is a simple 1D array, to
hold a 2D matrix with a given row length.  Assuming such a 2D table
containing multiple rows of sampled streams (for instance, the
amplitudes of a set of oscilators, sampled at a regular interval),
this opcode can extract one row (or a slice of a row) of that data
with linear interpolation between adjacent rows (if row is not a whole
number) and place the result in another table

## Syntax


```csound

tabrowlin krow, ifnsrc, ifndest, inumcols [, ioffset=0, istart=0, iend=0, istep=1

```
    
## Arguments

* **krow**: the row to read. It can be a fractional number, in which case the row will be linearly interpolated with the next row 
* **ifnsrc**: The table index to copy data from
* **ifndest**: The table index to copy data to (should be able to old one row of data)
* **inumcols**: The number of columns a row has, in the source table
* **ioffset**: an offset to where the 2D data starts (used to skip a header, if present)
* **istart**: start index to read from (refers to the row)
* **iend**: end index to read from the row (not inclusive, can't exceed inumcols)
* **istep**: step used to read along the row

## Output

This opcode has no outputs. The interpolated data is placed in **ifndest**


## Execution Time

* Performance

## Examples


```csound
<CsoundSynthesizer>
<CsOptions>
-odac     ;;;realtime audio out
</CsOptions>
<CsInstruments>


/* 

If reading out of bounds a PerformanceError will be raised. Because we
interpolate between rows, the last row that can be read is

  maxrow = (ftlen(ifnsrc)-ioffset)/inumcols - 2

*/

sr = 44100
ksmps = 128
nchnls = 1
0dbfs  = 1

instr 1
  ; just a simple test of the bare functionality
  ; generate a 4x3 table
  isource ftgentmp 0, 0, -12, -2, \
       0,  1,  2,  3,   \
      10, 11, 12, 13,   \
      20, 21, 22, 23
  ; create an empty table able to hold one row (4 elements)
  idest ftgentmp 0, 0, -4, -2, 0
  print ftlen(isource)
  ; we exceed the max. row to show what happens (the row is clipped
  ; to the max row possible and a message is printed to show the error)
  krow linseg 0, p3, 2.05
  printk2 krow, 20
  tabrowlin krow, isource, idest, 4
  ftprint idest, -1
endin

</CsInstruments>
<CsScore>
i 1 0 2
</CsScore>
</CsoundSynthesizer> 


```


## See also

* [getrowlin](getrowlin.md)
* [slicearray](http://www.csound.com/docs/manual/adsynt2.html)
* [beadsynt](beadsynt.md)

## Credits

Eduardo Moguillansky, 2019
