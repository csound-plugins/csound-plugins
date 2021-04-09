# pool_new

## Abstract

Create an empty  pool


## Description

A pool is actually a stack of numbers, based on an array.

The pool_ opcodes implement a stack of numbers. This is useful when assigning
ids to resources, like assigning fractional numbers to instrument instances to
access them individually. `pool_new` creates an empty pool. To use the pool it
must be first filled with values via `pool_push`. If a size is given, a pool of
the given size is allocated and the size can't be modified. If no size is given,
a pool of variable size is created.


!!! Note "Variable size / fixed size"

    A variable size pool produces allocations during performance, which might be
    a problem in certain situations / architectures or when running in `--realtime`
    mode. 


## Syntax

    ipool pool_new isize=0

### Arguments

* `isize`: the size of the pool. If leaved out or set as 0, a pool of variable
  size is created. Values can be pushed via `pool_push` and the pool will grow
  to accomodate these.

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

; create a global pool of fixed capacity
gipool pool_new 100

instr 1
  pool_push gipool, 45
  pool_push gipool, 47

  isize pool_size gipool
  print isize  

  inum1 pool_pop gipool
  inum2 pool_pop gipool

  print inum1
  print inum2

  isize pool_size gipool
  print isize  
  turnoff
endin

</CsInstruments>

<CsScore>
i1 0 1
e 5
; f0 3600

</CsScore>
</CsoundSynthesizer>


```

## See also

* [pool_gen](pool_gen.md)
* [pool_push](pool_push.md)
* [pool_pop](pool_pop.md)
* [pool_size](pool_size.md)
* [pool_capacity](pool_capacity.md)


## Credits

Eduardo Moguillansky, 2019
