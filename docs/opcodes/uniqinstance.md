# uniqinstance

## Abstract

Return an fractional instrument number which is not in use

## Description

Given an integer instrument number, `uniqinstance` a fractional 
instrument number which is not active now and can be used as p1
for `event`, `schedule` or similar opcodes to create a unique 
instance of the given instrument


!!! Note

    This opcode DOES NOT create a new instance. It just returns
    an instr number which can be used to create one
    

## Syntax

```csound

instrnum  uniqinstance integer_instrnum
instrnum  uniqinstance Sinstrname

```
    
### Arguments

* `integer_instrnum`: the integer instrument number
* `Sinstrname`: the name of a named instrument

### Output

* `instrnum`: a fractional instrument number which is guaranteed
  not to be active at the moment

### Execution Time

* Init 

## Examples


```csound


<CsoundSynthesizer>
<CsOptions>

--nosound
-m0

</CsOptions>

<CsInstruments>

/*
Example file for uniqinstance

instrnum  uniqinstance intinstr

Returns a unique fractional instrument number which is not
active at the moment and can be assigned to a new instance

*/

instr exit
  exitnow
endin

instr 1
  ; generate new instances manually, to check that 
  ; uniqinstance does not collide with existing instances
  ; scheduled via other means
  kcounter init 0
  ktrig metro 20
  
  if ktrig == 1 then
    kcounter += 1
    kinst = 10 + kcounter/100
    printsk "kinst=%f \n", kinst
    schedulek(kinst, 0, 1)
  endif
endin

instr 2
  instrnum10 uniqinstance 10
  prints "Unique instance of 10= %f\n", instrnum10
  instrnum11 uniqinstance 11, 1000
  prints "Unique instance of 11= %f\n", instrnum11
  turnoff
endin

instr 10
  print p1
endin

instr 11
  print p1
endin

instr example1
  printf ">>>>>>>>>>>>>>>>>>> example1 \n", 1
  schedule 1, 0, 0.5
  schedule 2, 0.5, 0.1
  schedule 10.150, 0, 0.1
  schedule 11, 0, 2
  turnoff
endin

; --------------------------------------
; Test that instances get recycled
instr example2
  prints ">>>>>>>>>>>>>>>>>>> example2 \n"
  i0 = 0
  istep = 0.01
  imaxinstances = 100
  idur = istep * imaxinstances
  while i0 < 1000 do
    schedule "scheduniq", i0*istep, idur, 20, imaxinstances
    i0 += 1
  od
  imaxdur = 2000 * istep + idur
  turnoff
endin

instr scheduniq
  inum = p4
  imax = p5
  inum2 = uniqinstance(inum, imax)
  if inum2 < 0 then
    prints "<<<<< Could not find unique instance >>>>>\n"
  else
    schedule inum2, 0, p3
    prints "active now=%d, inum=%f \n", active(inum), inum2
  endif
  turnoff
endin

instr 20
  prints "started %f\n", p1
  defer "prints", "finished %f \n", p1
endin

; -----------------------
; Text what happens if called with a non-existent instr
instr example3
    inum = uniqinstance(234)
    print inum
    turnoff
endin

</CsInstruments>

<CsScore>

; i "example1" 0 10 
; i "example2" 0 10
i "example3" 0 0.1

</CsScore>
</CsoundSynthesizer>



```


## See also

* [pread](pread.md)
* [pset](https://csound.com/docs/manual/pset.html)
* [p](https://csound.com/docs/manual/p.html)
* [passign](https://csound.com/docs/manual/passign.html)
* [nstrnum](https://csound.com/docs/manual/nstrnum.html)

## Credits

Eduardo Moguillansky, 2019
