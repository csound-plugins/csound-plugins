<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; test dict_print

ksmps = 64
nchnls = 2

instr 1
    idict dict_new "if"
    kcnt = 0
    kt timeinstk
    while kcnt < 100 do
      kkey = kcnt + kt*10000
      dict_set idict, kcnt, kkey*2
      kcnt += 1
    od

    dict_print idict, 1

    turnoff
endin


instr 2
  idict dict_new "sf"
  print idict
    kcnt = 0
    kt timeinstk
    while kcnt < 100 do
      kkey = kcnt + kt*10000
      dict_set idict, sprintfk("key_%d", kcnt), kkey*2
      kcnt += 1
    od

    dict_print idict, 1
    ; turnoff
endin



</CsInstruments>
<CsScore>
i 1 0 1
i 2 0.01 1
; i 3 0.02 1
; i 4 0.03 1


</CsScore>
</CsoundSynthesizer>
