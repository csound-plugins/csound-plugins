<CsoundSynthesizer>
<CsOptions>
-odac 

</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
  asig oscili 0.5, 1000
  aenv linseg 0, 0.1, 1, 1.9, 1, 1, 0 ; total 0.1+1.9+1=3
  asig *= aenv
  kfinished = detectsilence(asig, db(-90), 0.1)
  kms = timeinsts() * 1000
  if metro(30) == 1 then
    printsk "\r>>> Elapsed time: %.2f ms, env: %.5f         ", kms, aenv[0]
  endif
  if kfinished == 1 then
    println ""
    turnoff
  endif

  ip1 = p1
  idata = dict_new("sf", "instrnum", ip1, "elapsed", 0.)
  dict_set idata, "elapsed", kms
  atstop "elapsed", 0, 0, idata 
endin

instr elapsed
  idict = p4
  instrnum = dict_get:i(idict, "instrnum")
  ims = dict_get:i(idict, "elapsed")
  prints ">>> Instrument %d exited after %d ms\n", instrnum, ims
endin

</CsInstruments>

<CsScore>

i1 1 6
; f0 3600

</CsScore>
</CsoundSynthesizer>
