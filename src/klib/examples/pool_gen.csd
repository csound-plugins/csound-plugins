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
  ; create a pool, fill with with numbers from 1 to 100 (inclusive)
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