# pwriten

## Abstract

Modify pfield values of an active instrument instance created via nstance

## Description

`pwriten` can be used to modify the value of a pfield of a running instance 
created via `nstance`. It is similar to [pwrite](pwrite.md) but uses the 
instance id as returned via `nstance` instead of a fractional p1


### Dynamic pfields

In order for pwrite to have any effect, the instrument in question should
have an assignment of the sort `kfreq = p5`. 


!!! warning

    Setting a value of a pfield out of range will result in a 
    performance error. 

## Syntax

```csound

pwriten kinstanceid, index:i|k, value:i|k, [index2, value2, ...]

```
    
### Arguments

* `kinstanceid`: the instance number (as returned via nstance)
* `index` (i-, k-): the index of the pfield to modify. If kindex is 4, then p4 will be modified
* `value` (i-, k-): the new value of the given pfield


!!! info

    Multiple index:value pairs can be modified simultaneously


### Execution Time

* Performance

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
  
;; Example 1. instr 2 controls instr 1
  
instr 1
  pset 0, 0, 0, 40, 50
  kt timeinsts
  k4 = p4
  k5 = p5
  printf "time: %.4f\tinstance: %.3f\tp4: %f\tp5: %f \n", metro(20), kt, p1, k4, k5
endin

instr 2
  ieventid = p4
  kval line 0, p3, 1
  pwriten ieventid, 4, kval
  pwriten ieventid, 5, kval*2
endin

instr example1
  ieventid nstance 1, 0, 10, 40, 50
  if timeinstk() == 1 then
  	keventid = ieventid
  	schedulek 2, 0, 10, keventid
  endif
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
  aout *= linsegr(0, 0.1, 1, 0.1, 0)
  outs aout, aout  
endin

instr ex2_control
  iglissdur = p4
  kfreq1 linseg ntof("4A"), iglissdur, ntof("3A")
  kfreq2 linseg ntof("4F"), iglissdur, ntof("3F")
  iamp = 0.2
  kid1 = nstance:i("ex2_generator", 0, p3, iamp)
  kid2 = nstance:i("ex2_generator", 0, p3, iamp)
  pwriten kid1, 5, kfreq1
  pwriten kid2, 5, kfreq2
endin

instr example2
  schedule "ex2_control", 0, 8, 4
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

* [pwrite](pwrite.md)
* [pread](pread.md)
* [pset](https://csound.com/docs/manual/pset.html)
* [p](https://csound.com/docs/manual/p.html)
* [passign](https://csound.com/docs/manual/passign.html)
* [uniqinstance](uniqinstance.md)

## Credits

Eduardo Moguillansky, 2022
