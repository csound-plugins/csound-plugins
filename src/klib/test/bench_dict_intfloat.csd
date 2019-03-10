<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; benchmark khash vs array

ksmps = 64
nchnls = 2

instr 1
    idict dict_new "if"
    kcnt = 0
    kt timeinstk
    while kcnt < 1000 do
      kkey = kcnt + kt*10000
      ; dict_set idict, kkey, kkey*2
      dict_set idict, kcnt, kkey*2
      
      kcnt += 1
    od

    kcnt = 0
    while kcnt < 1000 do
      ; kkey = kcnt + kt*10000
      kkey = kcnt
      kval dict_get idict, kkey
      ; printf "key=%d    val=%f \n", kcnt, kkey, kval
      kcnt += 1
    od
endin

instr 2
  kxs[] init 1000
  kt timeinstk
  kcnt = 0
  while kcnt < 1000 do
    kxs[kcnt] = kcnt * 2
    kcnt += 1
  od

  kcnt = 0
  while kcnt < 1000 do
    kval = kxs[kcnt]
    ; printf "key=%d    val=%f \n", kcnt, kcnt, kval
    kcnt += 1
  od
endin
      

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
