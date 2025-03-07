<CsoundSynthesizer>
<CsOptions>
-odac           

</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

;; Example 1
instr 1
  printk2 timeinsts()
  ;; this will be called at the end of this instrument
  defer "event_i", "i", 2, 0, 1, 93
  defer "prints", "At end of instr 1: numcycles = %d\n", eventcycles()

endin

instr 2
  print p4
  turnoff
endin

instr example1
  schedule 1, 0, 0.1
  turnoff
endin

;; -------------------------------------------
;; Example 2

instr 10
  idict dict_new "*sf", "foo", 100, "bar", 200
  ;; delete this key at the end of the note
  defer "dict_del", idict, "foo"
  prints "Dict before delete\n"
  dict_print idict
  schedule 11, 0.5, 1, idict
  turnoff
endin 

instr 11
printf "Instr 11 \n", 1
  idict = p4
  ;; by now the dict should have deleted the key "foo"
  prints "Dict after delete\n"
  dict_print idict
  turnoff
endin

instr example2
  schedule 10, 0, 1
endin

schedule "example1", 0, 0.1

</CsInstruments>

<CsScore>
e 3
; f0 3600

</CsScore>
</CsoundSynthesizer>
