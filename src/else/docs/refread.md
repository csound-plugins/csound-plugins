# refread

## Abstract

Create a **read-only** view on the original object

## Description


`ref` and `refread` implement a mechanism to forward an object across events,
to reference objects inside other objects, etc. A `ref` generates a handle
wrapping the given object and returns an integer which maps to this handle. This
handle index can then be passed to any event, placed inside a collection (array,
dict, channel, etc.).

!!! Note

    `deref`, `refread` **does not** create an alias of the original object
    but generates a **read only** *copy* of it.


```csound

instr 1
  asig vco2 0.5, 1000
  kcutoff linseg 4000, p3, 200
  schedule 2, 0, p3, ref(asig), ref(kcutoff)
endin

instr 2
  ain     refread p4
  kcutoff refread p5
  iQ = 0.8
  asig K35_lpf ain, kcutoff, iQ
  outs asig, asig
  ; the references will be deallocated after this event finishes
endin

```


## Syntax

    kvar refread iref
    avar refread iref


### Arguments

* `iref`: a reference index as created via `ref`

### Output

* `xvar`: a read-only view over the object originally passed to `ref`


## Execution Time

* Init
* Performance

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
-m0
-d
</CsOptions>
<CsInstruments>

/*

Example file for ref / deref / refinc / refread / refvalid

*/

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
    prints "\n    >>> The reference has become invalid <<< \n"
  endif
  turnoff
endin

;; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

; Example 2: move semantics for arrays
instr 3
  ; create a source array
  kXs[] fillarray 1, 1, 2, 3, 5, 8, 13

  ; create a reference
  iref ref kXs

  ; In order to bridge the time gap between the end of life of the source
  ; of a ref and the scheduled event where a deref is taken, it is possible
  ; to create a forward reference, a "promise" that one deref has been scheduled
  ; in the future.
  ; Such forward refs are created via `refinc` (for increment reference).
  ; When this event stops, the memory ownership is transfered to the
  ; ref itself. The next `deref` will clear the forward ref and
  ; the memory is deallocated at the end of the deref's event.
  refinc iref
  schedule 4, p3+1, -1, iref

  ; The same can be compressed into one action by setting the forward
  ; references at creation time. The second argument to ref indicates the number
  ; of forward references to create. 
  schedule 4, p3+2, -1, ref(kXs, 1)
  defer "prints", " <<< instr. 3 finished >>> \n"
endin

instr 4
  prints "instr. 4"
  iView[] deref p4
  printarray iView
  ; At deinition time the memory of the `iView` array is marked as deallocated.
  ; The handle (a global structure created by the `ref` opcode) which owns the memory,
  ; is signaled that no other clients  of this data are alive. It deallocates the
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
  inum = 1000000
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
  ; test / k arrays
  ; 1. A way to convert a i-array to a k-array by taking a reference
  iXs[] genarray 0, 99
  kXs[] deref ref(iXs)
  
  kXs[0] = timeinsts()
  printarray kXs, metro(4)
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

; refs can be used to communicate between instrs.
; In this case, the memory of the array is moved to the handle
; so it is only deallocated when all other derefs go out of scope
instr 10
  kXs[] genarray_i 0, 9
  schedule 11, 0.5, 3, ref(kXs, 1)
  kXs[0] = linseg(0, p3, 1)
  defer "prints", "<<<< instr. 10 deallocated >>>> \n"
endin

instr 11
  kIn[] deref p4
  k1 = kIn[0]
  printf "time: %f, k1: %f \n", accum(metro(20)), timeinsts(), k1
endin

; ref / refread
; refs can be used to create communication channels
; between events
instr 20
  kfreq linseg 400, p3, 1000
  ; notica that instrument 21 outlives this instrument
  schedule 21, 0, 4, ref(kfreq)
  defer "prints", "<<<< instr. 20 deallocated >>>> \n"
endin

instr 21
  kfreq refread p4
  printf "kfreq=%f \n", accum(metro(20)), kfreq
  defer "prints", "<<<< instr 21 deallocated >>>> \n"
endin

instr 30
  asig vco2 0.1, 220
  schedule 31, 0, 2, ref(asig)
  defer "prints", "<<<< instr. 30 deallocated >>>> \n"
endin

instr 31
  ain refread p4
  kcutoff linseg 10000, p3, 1000
  asig K35_lpf ain, kcutoff, 8.0
  outs asig, asig
endin


; schedule 1, 0, 1
; schedule 3, 0, 1
; schedule 5, 0, 0.1
schedule "testUdoPerformance1", 0, 0.1
; schedule 7, 0, 0.1
; schedule 9, 0, 1
; schedule 10, 0, 2
; schedule 20, 0, 2
; schedule 30, 0, 2
</CsInstruments>

<CsScore>
e 10 

</CsScore>
</CsoundSynthesizer>

```


## See also

* [ref](ref.md)
* [defer](defer.md)
* [schedule](http://www.csounds.com/manual/html/schedule.html)
* [event](http://www.csounds.com/manual/html/event.html)
* [release](http://www.csounds.com/manual/html/release.html)

## Credits

Eduardo Moguillansky, 2019
