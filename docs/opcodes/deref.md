# deref

## Abstract

Dereference a previously created reference to a variable

## Description


`ref` and `deref` implement a mechanism to pass a reference to any object,
allowing to share a variable across instruments, with opcodes, etc. A reference
is a proxy to an axisting variable / array. A reference is reference counted and
deallocates itself when it falls out of scope without being referenced by any
object. It makes it possible to pass arrays by reference to user defined
opcodes, allowing to modify an array inplace, to skip copying memory, etc. With
a reference it is possible also to control an event from another event.

## Syntax

    xArray deref iref
    xvar   deref iref


### Arguments

* `iref`: a reference index as created via `ref`

### Output

* `xArray` / `xvar`: are created as a view of the object originally passed to `ref`


## Execution Time

* Init

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
-m0
-d
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

giA[] fillarray 0, 1000, 2000, 3000

; test: ref 
instr 1
  iXs[] fillarray 0, 10, 20, 30, 40
  iref ref iXs
  iYs[] deref iref
  printarray iYs, "", "instrument 1, iYs"
  iZs[] fillarray 0, 1, 3, 5, 7

  schedule 2, 0, 1, ref(iZs)

  ; this should fail since the source array is already
  ; gone when this instr. becomes active
  schedule 2, 1, 1, ref(iZs)
  
  turnoff
endin

instr 2
  iref = p4
  if ref_valid(iref) == 1 then
    iZs[] deref iref
    printarray iZs, "", "p1=2, iZs"
  else
    prints "\n    >>> The reference has become invalid <<< \n"
  endif
  turnoff
endin

; test: move ref
instr 3
  kXs[] fillarray 1, 1, 2, 3, 5, 8, 13
  ; A move operation. Now the reference owns the array. When iXs goes out
  ; of scope at the end of this event, its memory is not deallocated
  ; since it is now owned by the reference. It can be accessed later
  ; by "deref"
  ; NB - global arrays cannot be moved
  iref ref kXs, 1
  schedule 4, 1, -1, iref
  endif
  ; turnoff
endin

instr 4
  iref = p4
  iXs[] deref iref
  iYs[] deref iref
  prints "\n p1=4 iXs="
  printarray iXs
endin

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

opcode arrayadd, i[], i[]i
  iIn[], ix xin
  iOut[] = iIn + ix
  xout iOut
endop

opcode arrayaddref, i[], ii
  iref, ix xin
  iIn[] deref iref
  iOut[] = iIn + ix
  xout iOut
endop

opcode arrayadd_inplace, 0, ii
  iref, ix xin
  iIn[] deref iref
  iIn += ix
endop

opcode arrayadd_inplace2, 0, iii
  irefin, irefout, ix xin
  iIn[] deref irefin
  iOut[] deref irefout
  iOut = iIn + ix
endop

instr testUdoPerformance1
  inum = 1000000
  iXs[] genarray 0, inum
  ii = 0
  it0 rtclock
  while ii < 20 do
    iYs[] arrayadd iXs, 2.0
    ii += 1
  od
  it1 rtclock
  prints "Dur UDO=%.8f \n", it1 - it0

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
  prints "Dur UDO ref=%.8f \n", it1 - it0

  iZs[] genarray 0, inum
  it0 rtclock
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  arrayadd_inplace ref(iZs), 1
  it1 rtclock
  prints "Dur UDO inplace=%.8f \n", it1 - it0
  ; printarray iZs
endin

instr 7
  iIn[] genarray 0, 1000
  iOut[] init lenarray(iIn)
  arrayadd_inplace2 ref(iIn), ref(iOut), 0.5
  printarray iOut
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
; In this case, the array is moved to the handle so it is
; only deallocated when all other derefs go out of scope
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

instr 20
  kfreq linseg 400, p3, 1000
  ; notica that instrument 21 outlives this instrument
  schedule 21, 0, 4, ref(kfreq)
  defer "prints", "<<<< instr. 20 deallocated >>>> \n"
endin

instr 21
  kfreq deref p4
  printf "kfreq=%f \n", accum(metro(20)), kfreq
  defer "prints", "<<<< instr. 21 deallocated >>>> \n"
endin

instr 30
  asig vco2 0.1, 220
  schedule 31, 0, 2, ref(asig)
  defer "prints", "<<<< instr. 30 deallocated >>>> \n"
endin

instr 31
  ain deref p4
  kcutoff linseg 10000, p3, 1000
  asig K35_lpf ain, kcutoff, 8.0
  outs asig, asig
endin

;; Example 1: a reference is valid while the source is valid
; schedule 1, 0, 1

;; Example 2: if a ref is marked as a move operation, the reference
;; owns the data and can outlive the source
schedule 3, 0, 1

; schedule 5, 0, 0.1

; schedule "testUdoPerformance1", 0, 0.1
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
