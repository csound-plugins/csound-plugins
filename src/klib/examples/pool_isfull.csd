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
  pool_push i1
  if pool_isfull(ipool) == 1 then
    prints "pool is full\n"
  endif
  ; this should fail
  pool_push 10
  turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1

</CsScore>
</CsoundSynthesizer>