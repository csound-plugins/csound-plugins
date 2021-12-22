# pool_size

## Abstract

Returns the size of a pool

## Description

A pool is actually a stack of numbers, based on an array.

The pool_ opcodes implement a stack of numbers. This is useful when assigning
ids to resources, like assigning fractional numbers to instrument instances to
access them individually. To get a value, call `pool_pop` and when finished
using it the value is returned to the pool with `pool_push`. `pool_size` returns
the size of the pool (the number of items in it, not the capacity, see
`pool_capacity` for that)

!!! Note

    To differenciate between the opcode being called at init- or at performance
    time, use the functional style `pool_size:i(ipool)` or `pool_size:k(ipool)`

## Syntax

    isize pool_size ipool
    ksize pool_size kpool

### Arguments

* `ipool`: the pool to push the value to

### Output

* `isize` / `ksize`: the number of items in the pool


### Execution Time

* Init
* Performance

## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

/*
   Example file for pool_size

   pool_size returns the size of the pool, either at 
   init or at performance time

   The size of a pool is the number of items actually inside
   the pool (see also pool_capacity)

*/

opcode print_pool, 0, i
  ipool xin
  i0 = 0
  isize = pool_size(ipool)
  while i0 < isize do
    item = pool_at(ipool, i0)
    print item
    i0 += 1
  od
endop

instr 1
  ipool pool_gen 10
  i1 pool_pop ipool
  i2 pool_pop ipool
  prints "\n<<< pool size: %d, pool capacity: %d >>> \n\n", \
         pool_size:i(ipool), pool_capacity:i(ipool)
  print_pool ipool
  turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1

</CsScore>
</CsoundSynthesizer>


```

## See also

* [pool_capacity](pool_capacity.md)
* [pool_pop](pool_pop.md)
* [pool_new](pool_new.md)
* [pool_gen](pool_gen.md)


## Credits

Eduardo Moguillansky, 2019
