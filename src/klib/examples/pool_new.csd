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

; create an empty global pool of fixed capacity. 
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