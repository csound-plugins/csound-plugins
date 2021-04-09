# pool_isfull

## Abstract

Returns 1 if the pool is full

## Description

A pool is actually a stack of numbers, based on an array.

The pool_ opcodes implement a stack of numbers. This is useful when assigning
ids to resources, like assigning fractional numbers to instrument instances to
access them individually, assigning bus indexes, etc. To get a value, call `pool_pop` 
and when finished using it the value is returned to the pool with `pool_push`. 

`pool_isfull` returns 1 if the pool is full (its size == its capacity) and the pool can't 
be grown. 

!!! Note

    Use `pool_size(ipool) == 0` to query if the pool is empty
    

!!! Note

    To differenciate between the opcode being called at init- or at performance
    time, use the functional style `pool_isfull:i(ipool)` or `pool_isfull:k(ipool)`

## Syntax

    i_isfull pool_isfull ipool
    k_isfull pool_isfull ipool

### Arguments

* `ipool`: the pool to push the value to

### Output

* `i_isfull` / `k_isfull`: 1 if the pool is full, 0 otherwise


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
   Example file for pool_isfull

   pool_isfull returns 1 if the pool is full, 0 otherwise
   
   If the pool was created
   The size of a pool is the number of items actually inside
   the pool (see also pool_capacity)

*/

instr 1
  ; create a pull of fixed size, filled with the integers 0 to 9
  ipool pool_gen 10
  i1 pool_pop ipool
  pool_push ipool, i1
  
  if pool_isfull:i(ipool) == 1 then
    prints "pool is full\n"
  endif
  ; this should fail
  pool_push ipool, 10
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
* [pool_capacity](pool_capacity.md)
* [pool_pop](pool_pop.md)
* [pool_new](pool_new.md)
* [pool_gen](pool_gen.md)



## Credits

Eduardo Moguillansky, 2019
