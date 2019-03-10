<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; This is used together with the script 'bench-get'
 
ksmps = 64
nchnls = 2

gidict init 0

#ifndef NUMKEYS
#define NUMKEYS #50000#
#endif

instr 1
  ; tests dict_set performance
  idict dict_new "sf", 1  ;; a global dict
  gidict = idict
  kcnt = 0
  while kcnt < $NUMKEYS do
    Sbar = sprintfk("bar%d", kcnt)
    dict_set idict, Sbar, kcnt
    chnset kcnt, Sbar
    kcnt += 1
  od
  turnoff
endin

instr 2
  kcnt = 0
  while kcnt < 1000 do
    kkey = int(rnd:k($NUMKEYS))
    Skey = sprintfk("bar%d", kkey)
    kval chnget Skey
    ; printf "%s : %f \n", kcnt, Skey, kval
    kcnt += 1
  od
endin

instr 3
  kcnt = 0
  while kcnt < 1000 do
    kkey = int(rnd:k($NUMKEYS))
    Skey = sprintfk("bar%d", kkey)
    kval dict_get gidict, Skey
    ; printf "%s : %f \n", kcnt, Skey, kval
    kcnt += 1
  od
endin


schedule $INSTRNUM, 0.01, 4
    
</CsInstruments>
<CsScore>
i 1 0 0.1
; i 2 0 1
f 0 2

</CsScore>
</CsoundSynthesizer>
