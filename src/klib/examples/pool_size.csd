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