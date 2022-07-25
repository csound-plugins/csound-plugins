# pool_capacity

## Abstract

Returns the capacity of a pool

## Description

A pool is actually a stack of numbers, based on an array. `pool_capacity`
returns the maximum number of items which can be pushed in this pool. If the
pool was created with a fixed capacity, then pushing when the pool is full will
result in a performance error. To create a pool with dynamic size, use pool_new
without giving a capacity (see [pool_new](pool_new.md))

The pool_ opcodes implement a stack of numbers. This is useful when assigning
ids to resources, like assigning fractional numbers to instrument instances to
access them individually. To get a value, call `pool_pop` and when finished
using it the value is returned to the pool with `pool_push`. `pool_capacity` returns
the capacity of the pool (the max. number of items it can hold, not the actual
size, see [pool_size](pool_size.md) for that)

!!! Note

    To differenciate between the opcode being called at init- or at performance
    time, use the functional style `pool_capacity:i(ipool)` or `pool_capacity:k(ipool)`

## Syntax

    icapacity pool_capacity ipool
    kcapacity pool_capacity kpool

### Arguments

* `ipool`: the pool query

### Output

* `icapacity` / `kcapacity`: the max. number of items this pool can hold


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

instr 1
  ipool pool_gen 10
  i1 pool_pop ipool
  i2 pool_pop ipool
  prints "\n<<< pool size: %d, pool capacity: %d >>> \n\n", \
         pool_size:i(ipool), pool_capacity:i(ipool)
  turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1

</CsScore>
</CsoundSynthesizer>


```

## See also

* [pool_size](pool_size.md)
* [pool_pop](pool_pop.md)
* [pool_new](pool_new.md)
* [pool_gen](pool_gen.md)


## Credits

Eduardo Moguillansky, 2019
