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