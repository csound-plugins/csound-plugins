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

; create a global pool for all instances
gipool pool_gen 1, 1000

; create audio buses
ginumbuses = 200
zakinit ginumbuses, 1

; a pool of buses
gibuses pool_gen ginumbuses

; release item back to instance pool, print a message
; when that happens
opcode pool_release_instance, 0, ii
  ipool, ip1 xin
  ival = frac2int(ip1, pool_capacity(ipool))
  defer "prints", "releasing back to pool %d \n", ival
  pool_push ipool, ival, 1
endop

instr exit
  exitnow
endin

; schedule and control audio generator
instr controlsaw
  ; get an unused bus
  ibus = pool_pop(gibuses)
  prints "Using bus %d\n", ibus
  ifreq = p5
  icap pool_capacity gipool

  ; get a unique instance number for "saw" instrument
  isaw = nstrnum("saw") + pool_pop(gipool) / icap
  schedule isaw, 0, p3, ibus, ifreq

  ; modulate the frequency
  pwrite isaw, 5, linseg(ifreq, p3, ifreq*0.1)

  ; get a unique instance number for filter instrument
  ifilter = nstrnum("filter") + pool_pop(gipool) / icap
  schedule ifilter, 0, p3, ibus, 100

  ; modulate its cutoff freq.
  pwrite ifilter, 5, linseg(100, p3, 4000)

  ; release bus back to pool when finished (notice the 1 at the end)
  ; we could have done
  ;  defer "pool_push", gibuses, ibus
  pool_push gibuses, ibus, 1
endin


instr saw
  ibus = p4
  kfreq = p5
  iamp  = 0.02
  ifade = 0.05
  a0  = vco2(iamp, kfreq)
  a0 += vco2(iamp, kfreq * 0.5)
  a0 += vco2(iamp, kfreq * 0.25)
  a0 *= cosseg(0, p3*0.5, 1, p3*0.5, 0)
  
  ; write audio to bus
  zawm a0, ibus
  
  ; release instance back to the pool
  pool_release_instance gipool, p1
endin

; filter audio in bus, output to the outside world
instr filter
  ibus = p4
  kfreq = p5
  a0 zar ibus
  a0 zdf_ladder a0, kfreq, 18
  outch 1, a0
endin

instr +example1
  idur = 10                          ; dur of each note
  ioffset = 0.6                      ; offset between notes
  iFreqs[] genarray 1000, 8000, 100  ; begin freq. of gliss. for each note

  printarray iFreqs

  ; create a note for each frequency
  i0 = 0
  while i0 < lenarray(iFreqs) do
    schedule "controlsaw", ioffset*i0, idur, 0, iFreqs[i0]
    i0 += 1
  od

  ; clear zak 
  zacl 0, ginumbuses

  ; schedule exit
  schedule "exit", ioffset*i0 + idur + 1, -1
endin

schedule "example1", 0, -1

</CsInstruments>

<CsScore>
</CsScore>

</CsoundSynthesizer>
