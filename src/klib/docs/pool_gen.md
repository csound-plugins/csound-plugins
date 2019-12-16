# pool_gen

## Abstract

Create a pool and fill it with values


## Description

A pool is actually a stack of numbers, based on an array.

The pool_ opcodes implement a stack of numbers. This is useful when assigning
ids to resources, like assigning fractional numbers to instrument instances to
access them individually. `pool_gen` creates a pool of values of a given size by
filling the pool with values from 1 to the given size

## Syntax

    ipool pool_gen isize
    ipool pool_gen istart, iend


### Arguments

* `isize`: the size of the pool.
* `istart`: the start item
* `iend`: the end item (inclusive)

### Output

* `ipool`: an index identifying this pool. This index is used when calling any
  `pool` opcode

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
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

opcode pool_print, 0, i
  ipool xin
  i0 = 0
  isize = pool_size(ipool)
  while i0 < isize do
    ival = pool_at(ipool, i0)
    print ival
    i0 += 1
  od
endop

instr test1
  ipool pool_gen 100

  isize pool_size ipool
  icapacity pool_capacity ipool
  print isize
  print icapacity
  pool_print ipool
endin

instr test2
  ipool pool_gen 1, 100

  isize pool_size ipool
  icapacity pool_capacity ipool
  print isize
  print icapacity
  pool_print ipool
endin

instr sep
  prints "\n--------------------------------\n\n"
  ; turnoff
endin

</CsInstruments>

<CsScore>
i "test1" 0   0.1
i "sep"   0.1 0.1
i "test2" 0.2 0.1

</CsScore>
</CsoundSynthesizer>

```

## See also

* [pool_new](pool_new.md)
* [pool_push](pool_push.md)
* [pool_pop](pool_pop.md)
* [pool_size](pool_size.md)
* [pool_capacity](pool_capacity.md)


## Credits

Eduardo Moguillansky, 2019
