# pool_free

## Abstract

Deletes a pool, frees its memory

## Description

A pool is actually a stack of numbers, based on an array.

The pool_ opcodes implement a stack of numbers. This is useful when assigning
ids to resources, like assigning fractional numbers to instrument instances to
access them individually. `pool_free` deletes a pool created via [pool_new](pool_new.md) or [pool_gen](pool_gen.md)

## Syntax

	pool_free ipool, iwhen=0

### Arguments

* `ipool`: the pool to free
* `iwhen`: when to free it. 0=at init time, 1=at the end of the event

### Execution Time

* Init
* Deinit

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

;;  pool_free ipool, iwhen=0 
;;  * iwhen: 0 = free at init time, 1 = free at deinit time

gipool pool_gen 1, 25

pool_free gipool, 1

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

instr test2
  ipool pool_gen 100
  pool_print ipool
  pool_free ipool
  ; This should fail
  isize = pool_size(ipool)
endin

instr test1
  ipool pool_gen 100
  pool_print ipool
  pool_free ipool, 1
endin


</CsInstruments>

<CsScore>
i "test1" 0   0.1
; i "test2" 0.2 0.1

</CsScore>
</CsoundSynthesizer>



```

## See also

* [pool_new](pool_new.md)
* [pool_gen](pool_gen.md)


## Credits

Eduardo Moguillansky, 2025
