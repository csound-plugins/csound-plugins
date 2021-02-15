# deref

## Abstract

Dereference a previously created reference to a variable

## Description

`ref` and `deref` implement a mechanism to pass a reference to any object,
allowing to share a variable across instruments, with opcodes, etc. A ref is reference 
counted and deallocates itself when it falls out of scope without being referenced by any
object. It makes it possible to pass arrays by reference to user defined
opcodes, allowing to modify an array inplace, skip copying memory, etc. 


## Syntax

    xArray deref iref, iextrarefs=0

### Arguments

* `iref`: a reference index as created via `ref`
* `iextrarefs`: extra references used, matching any extra reference allocated via `ref`
    (see [ref](ref.md) for more information)

### Output

* `xArray` / `xvar`: are created as a view of the object originally passed to `ref`


## Execution Time

* Init

## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>
<CsInstruments>

;; Example file for ref - deref

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

giA[] fillarray 0, 1000, 2000, 3000

; Example 1: take a ref from an array, deref it to create a second view of it 
instr 1
  ; a source array
  iX[] fillarray 0, 10, 20, 30, 40
  
  ; create a ref of iXs, return the index 
  iref ref iX
  ; now iYs points to iXs
  iY[] deref iref
  printarray iY, "", "instrument 1, iY"

  
  iZ[] fillarray 0, 1, 3, 5, 7

  ; create a ref, pass it to instr. 2
  schedule 2, 0, 1, ref(iZ)

  ; create another ref of iZ. In this case the event is scheduled
  ; in the future, so the source will not exist anymore when instr. 2
  ; is scheduled. This should fail.
  schedule 2, 1, 1, ref(iZ)
  
  turnoff
endin

instr 2
  iref = p4
  if refvalid(iref) == 1 then
    iZs[] deref iref
    printarray iZs, "", "p1=2, iZs"
  else
    prints "\n    The reference has become invalid\n"
  endif
  turnoff
endin

;; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

; Example 2: extra references to keep array alive
instr 3
  ; create a source array
  kXs[] fillarray 1, 1, 2, 3, 5, 8, 13

  ; In order to bridge the time gap between the end of life of the source
  ; of a ref and the scheduled event where a deref is taken, it is possible
  ; to create a forward reference, a "promise" that one deref has been scheduled
  ; in the future.

  ; short lived event, ends before this event
  schedule 4, 0, 0.1, ref(kXs), 0

  ; starts before we end, but survives us
  schedule 4, p3-0.1, 0.2, ref(kXs), 0

  ; starts after we end, we need an extra reference 
  schedule 4, p3+1, 0.1, ref(kXs, 1), 1
  
  defer "prints", "  --- instr. 3 finished --- \n"
endin

instr 4
  prints "instr. 4\n   "
  kView[] deref p4, p5
  printarray kView
  defer "prints", " --- instr. 4 finished --- \n"
  ; At deinition time the memory of the `iView` array is marked as deallocated.
  ; The handle (a global structure created by the `ref` opcode) which owns the memory,
  ; is signaled that no other clients of this data are alive. It deallocates the
  ; original memory and frees itself
endin

  ;; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  ; test: multiple derefs
instr 5
  iXs[] fillarray 0, 1, 4, 9
  iref ref iXs

  iYs[] deref iref
  iZs[] deref iref
  printarray iYs
  printarray iZs
  iXs[0] = 100
  printarray iZs
  turnoff
endin

; test performance of pass-by-value vs pass-by-reference
opcode arrayadd, i[], i[]i
  ; pass by value in and out
  iIn[], ix xin
  iOut[] = iIn + ix
  xout iOut
endop

opcode arrayaddref, i[], ii
  ; pass by ref in, by value out
  iref, ix xin
  iIn[] deref iref
  iOut[] = iIn + ix
  xout iOut
endop

opcode arrayadd_inplace, 0, ii
  ; in place 
  iref, ix xin
  iIn[] deref iref
  iIn += ix
endop

opcode arrayadd_byref_inout, 0, iii
  ; pass by ref in and out
  irefin, irefout, ix xin
  iIn[]  deref irefin
  iOut[] deref irefout
  if lenarray(iOut) >= lenarray(iIn) then
    iOut = iIn + ix
  endif
endop

instr testUdoPerformance1
  ; Here we test the performance gain of passing arrays by reference.
  ; Passing the input array by reference seems to produce a speedup of ~25%,
  inum = 10000
  iXs[] genarray 0, inum
  ii = 0
  it0 rtclock
  while ii < 20 do
    iYs[] arrayadd iXs, 2.0
    ii += 1
  od
  it1 rtclock
  prints "Dur UDO pass by value = %.8f \n", it1 - it0
                                                                                                                
  iref = ref(iXs)
  it0 rtclock
  iY0[] arrayaddref iref, 0.1
  iY0   arrayaddref iref, 0.2
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  iY0   arrayaddref iref, 0.3
  it1 rtclock
  prints "Dur UDO pass by ref input = %.8f \n", it1 - it0

  iZs[] genarray 0, inum
  iOut[] init lenarray(iZs)

  it0 rtclock
  irefZ = ref(iZs)
  irefOut = ref(iOut)  
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  arrayadd_byref_inout irefZ, irefOut, 0.5
  it1 rtclock
  
  prints "Dur UDO pass by ref in and out=%.8f \n", it1 - it0
  ; printarray iOut
endin

instr 7
  iIn[] genarray 0, 1000
  iOut[] init lenarray(iIn)
  arrayadd_byref_inout ref(iIn), ref(iOut), 0.5
  turnoff
endin

instr 8
  ; test k arrays
  ; 1. A way to convert a i-array to a k-array by taking a reference
  iXs[] genarray 0, 99
  kXs[] deref ref(iXs)
  
  kXs[0] = timeinsts()
  printarray kXs, metro(8)
endin

instr 9
  ; we need genarray_i because otherwise kXs is not initialized at i-time
  kXs[] genarray 0, 9
  iXs[] deref ref(kXs)
  iXs += 10
  printarray iXs
  printarray kXs
  turnoff
endin


; schedule 1, 0, 1
; schedule 3, 0, 1
; schedule 5, 0, 0.1
schedule "testUdoPerformance1", 0, 0.1
; schedule 8, 0, 4
</CsInstruments>

<CsScore>
e 10 

</CsScore>

</CsoundSynthesizer>



```


## See also

* [ref](ref.md)
* [defer](defer.md)
* [schedule](http://www.csound.com/docs/manual/schedule.html)
* [event](http://www.csound.com/docs/manual/event.html)
* [release](http://www.csound.com/docs/manual/release.html)

## Credits

Eduardo Moguillansky, 2019
