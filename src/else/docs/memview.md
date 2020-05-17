# memview

## Abstract

Create a view into a table or another array

## Description

memview makes it possible to operate on a table as if it
were an array (using any array opcode), or to operate on a slice
of an array without copying the underlying memory. It can be used to
efficiently read a slice of an existing array, operate on rows of a 
2D matrix, etc.

The underlying memory is shared between the source and the view
for the duration of the event.

Using the returned array as a left-side variable is not supported. This
can result in reallocation/resizing of the array, which in this
case is not allowed since the underlying memory does not belong to
the array. **Using it in this way results in undefined behaviour**

The same holds true for creating a view from a ftable and freeing
the ftable during the lifetime of the view.

## Syntax

```csound

iView[]  memview ift,      [, istart=0, iend=0]
kView[]  memview iSource[] [, istart=0, iend=0]
kView[]  memview kSource[] [, istart=0, iend=0]

```

### Arguments

* `ift`: the source ftable
* `iSource[] / kSource[]`: the source array
* `istart`: the start index of the view (default=0)
* `iend`: the end index of the view (non inclusive, default=end of
          the table / array)

### Output

* `iView[]` / `kView[]`: the array view

### Execution time

* Init

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
--nosound
-m0
-d
</CsOptions>

<CsInstruments>

/*
memview



Create a view into a table or another array

iView[]  memview ift,      [, istart=0, iend=0]
kView[]  memview iSource[] [, istart=0, iend=0]
kView[]  memview kSource[] [, istart=0, iend=0]

ift: the source ftable
iSource[] / kSource[]: the source array
istart: the start index of the view (default=0)
iend: the end index of the view (non inclusive, default=end of
the table / array)

memview makes it possible to operate on a table as if it
were an array, using any array opcode. 

It is also possible to take a slice from a different array
without copying the underlying elements. It can be used to 
efficiently read a slice of an existing array, operate on
rows of a 2D matrix, etc.

The underlying memory is shared between the source and the view
for the duration of the event.

It is not supported to reuse the array as a left-side variable
because that could incurr in reallocation / resizing, which in this
case is not allowed since the underlying memory does not belong to 
the array. Using it in this way results in undefined behaviour.

The same holds true for creating a view from a ftable and freeing 
the ftable during the lifetime of the view.


*/


gitab ftgen 0, 0, 10, -2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9

instr 1
  iView[] memview gitab
  prints "\n$$$$ Original array:"
  printarray iView
  iView[2] = 20
  iView *= 0.5
  prints "\n$$$$ Modified array:"
  printarray iView

  prints "\n$$$$ Source table should be modified as well\n"
  ftprint gitab
  
  prints "array length: %d \n", lenarray(iView)
  turnoff
endin

instr 2
  kView[] memview gitab
  printf "\n$$$$ Original array:", 1
  printarray kView
  kView[2] = 20
  kView *= 0.5
  printf "\n$$$$ Modified array:", 1
  printarray kView

  printf "\n$$$$ Source table should be modified as well\n", 1
  ftprint gitab, -1
  
  turnoff
endin

instr 3 
  iX[] fillarray 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
  iY[] memview iX, 5
  kZ[] memview iX, 3, 8
  iY *= 10
  printarray iY
  printarray iX
  printarray kZ
  turnoff
endin

instr 4 
  kX[] fillarray 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
  kY[] memview kX, 5
  kY *= 0.5
  printarray kX
  printarray kY
  turnoff
endin

</CsInstruments>

<CsScore>

; i1 0 1
; i2 0 1
i3 0 1
; f0 3600

</CsScore>
</CsoundSynthesizer>

```

## See also


* [copya2ftab](https://csound.com/docs/manual/copya2ftab.html)
* [slicearray](https://csound.com/docs/manual/slicearray.html)
* [tabrowlin](https://csound.com/docs/manual/tabrowlin.html)
* [ref](ref.md)
* [deref](deref.md)

## Credits

Eduado Moguillansky, 2019
