# pool_at

## Abstract

Returns the item of a pool at a given index

## Description

A pool is actually a stack of numbers, based on an array.

The pool_ opcodes implement a stack of numbers. This is useful when assigning
ids to resources, like assigning fractional numbers to instrument instances to
access them individually. To get a value, call `pool_pop` and when finished
using it the value is returned to the pool with `pool_push`. `pool_size` returns
the size of the pool (the number of items in it, not the capacity, see
`pool_capacity` for that)

## Syntax

    item  pool_at ipool, index
    kitem pool_at ipool, kindex
  
### Arguments

* `ipool`: the pool to push the value to
* `index` / `kindex`: the index to query

### Output

* `item` / `kitem`: the item at the given index


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
   Example file for pool_at

   pool_at returns the item of a pool at a given index

   item pool_at ipool, idx
   ktem pool_at ipool, kidx
   
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
