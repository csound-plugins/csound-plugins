# pwrite

## Abstract

Modify pfield values of an active instrument instance

## Description

`pwrite` can be used to modify the value of a pfield of a running instance 
(possibly a fractional instrument number).

A matching instance is searched at performance time, so that its
behaviour can be controlled via `if` or `timout` statements.

If no active instance is found, search is retried until a matching
instance is found. To avoid retrying, set `instrnum` to a negative value.

If the instance ceases to exist during another instrument is modifying
its pfield values, nothing happens. `pwrite` notices that the instance
is not active anymore and becomes a `NOOP`.

### Exact instance vs Broadcasting

If `instrnum` is a fractional instrument number, pwrite will only affect
the first instance matching this exact number.

If `instrnum` is set to an integer number, `pwrite` will **broadcast** the
changes to **ALL** instruments with the same integer number.

!!! Note

    Setting a value of a pfield out of range will result in a 
    performance error. 

## Syntax

    pwrite instrnum:i, index:i|k, value:i|k
    
### Arguments

* `instrnum`: the (fractional) instrument number to modify
* `iindex` / `kindex`: the index of the pfield to modify. 
  If kindex is 4, then p4 will be modified
* `ivalue` / `kvalue`: the new value of the given pfield

### Execution Time

* Init (if index and value are i-values)
* Performance (if either index or value are k-variables)

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>

-odac
-m0

</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

instr exit
  exitnow
endin
  
;; Example 1. instr 2 creates and controls instr 1
  
instr 1
  pset 0, 0, 0, 40, 50
  kt timeinsts
  k4 = p4
  k5 = p5
  printf "time: %.4f\tinstance: %.3f\tp4: %f\tp5: %f \n", metro(20), kt, p1, k4, k5
endin

instr 2
  kval line 0, p3, 1
  pwrite 1.01, 4, kval
  pwrite 1.02, 5, kval*2
endin

instr example1
  schedule 1.01, 0, 4, -1
  schedule 1.02, 0, 4, -1
  schedule 2,    1, 1
  schedule "exit", 4, -1
  turnoff
endin

;-----------------------------
; Example 2, one instrument modulates another

instr ex2_generator
  pset p1, p2, p3, 0.5, 1000, 4000, 0.1
  kamp       = p4
  kfreq      = p5
  kcutoff    = p6
  kresonance = p7
  asaw vco2, kamp, kfreq
  aout moogladder2, asaw, kcutoff, kresonance
  aout *= linsegr(0, 0.05, 1, 0.05, 0)
  outs aout, aout  
endin

instr ex2_control
  iglissdur = p4
  inum = nstrnum("ex2_generator")
  inum1 = inum + 0.001
  inum2 = inum + 0.002
  kfreq1 linseg ntof("4A"), iglissdur, ntof("3A")
  kfreq2 linseg ntof("4F"), iglissdur, ntof("3F")
  ;                      amp
  schedule inum1, 0, p3, 0.2 
  schedule inum2, 0, p3, 0.2
  pwrite inum1, 5, kfreq1
  pwrite inum2, 5, kfreq2
endin

instr ex2_broadcast
  printf "filter start\n", 1
  inum = nstrnum("ex2_generator")
  kcutoff    linseg 4000, p3, 400
  kresonance linseg 0.1, p3*0.5, 0.9
  pwrite inum, 6, kcutoff, 7, kresonance
endin

instr example2
  schedule "ex2_control", 0, 8, 4
  schedule "ex2_broadcast", 4, 4
  schedule "exit", 8.5, -1
  turnoff
endin

;; Uncomment as needed

; schedule "example1", 0, 1
schedule "example2", 0, 1

</CsInstruments>
<CsScore>

</CsScore>
</CsoundSynthesizer>


```


## See also

* [pread](pread.md)
* [pset](https://csound.com/docs/manual/pset.html)
* [p](https://csound.com/docs/manual/p.html)
* [passign](https://csound.com/docs/manual/passign.html)
* [uniqinstance](uniqinstance.md)

## Credits

Eduardo Moguillansky, 2019
