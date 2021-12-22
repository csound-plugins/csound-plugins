<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; Example for opcode bpf

/*

  bpf stands for Break Point Function

  Given an x value and a series of pairs (x, y), it returns
  the corresponding y value in the linear curve defined by the
  pairs

  It works both at i- and k- time

  ky    bpf kx,    kx0, ky0, kx1, ky1, kx2, ky2, ...
  kys[] bpf kxs[], kx0, ky0, kx1, ky1, kx2, ky2, ...

  NB: x values must be ordered (kx0 < kx1 < kx2 etc)

  See also: bpfcos, linlin, lincos
    
*/
  
ksmps = 64
nchnls = 2

instr 1
  idict dict_new "sf"
  dict_set idict, "bar", 123
  kfoo dict_get idict, "foo", 314
  kbar dict_get idict, "bar", 10
  printk2 kfoo
  printk2 kbar
endin

; test S -> S
instr 2
  idict dict_new "SS"
  kt timeinstk
  printk2 kt

  kcnt = 0
  while kcnt < 100 do

    dict_set idict, "bar", sprintfk("barvalue%d", kt)
    dict_set idict, "foo", sprintfk("foovalue%d", kt)
  
    Sbar dict_get idict, "bar"
    Sfoo dict_get idict, "foo"
    printf "bar: '%s'\n", kt, Sbar
    printf "foo: '%s'\n", kt, Sfoo
      kcnt += 1
    od
  
endin

instr 3
  ; idict dict_new "SS"
  kt timeinstk
  printk2 kt

  kcnt = 0
  while (kcnt < 100) do
    chnset sprintfk("barvalue%d", kt), "bar"
    chnset sprintfk("foovalue%d", kt), "foo"
    Sbar chnget "bar"
    Sfoo chnget "foo"
    
    printf "bar: '%s'\n", kt, Sbar
    printf "foo: '%s'\n", kt, Sfoo
      kcnt += 1
    od
    
endin

; compare khash to chnget/set
; test S -> f
instr 10
  idict dict_new "sf"
  kt timeinstk
  printk2 kt

  kcnt = 0
  kbar init 0
  kfoo init 0
  while kcnt < 2000 do
    dict_set idict, "bar", kt*10000 + kcnt
    dict_set idict, "foo", kt*10000 + kcnt*2
  
    kbar dict_get idict, "bar"
    kfoo dict_get idict, "foo"
    ; printf "bar: %f   foo: %f\n", kt, kbar, kfoo
      
      kcnt += 1
    od
  
endin

; compare khash to chnget/set
; test S -> f
instr 11
  idict dict_new "sf"
  kt timeinstk
  printk2 kt

  kcnt = 0
  kbar init 0
  kfoo init 0
  
  while kcnt < 2000 do
    Skey1 = sprintfk("bar%d", kcnt)
    Skey2 = sprintfk("foo%d", kcnt)
    dict_set idict, Skey1, kt*10000 + kcnt
    dict_set idict, Skey2, kt*10000 + kcnt*2
  
    kbar dict_get idict, Skey1
    kfoo dict_get idict, Skey2
    ;printf "bar: %f   foo: %f\n", kcnt, kbar, kfoo
      
      kcnt += 1
    od
  
endin

instr 12
  ; tests dict_get performance
  idict dict_new "sf"
  kt timeinstk
  ; printk2 kt

  kcnt = 0
  kbar init 0
  kfoo init 0
  if kt > 1 kgoto perf
  dict_set idict, "bar", kt
  dict_set idict, "foo", 2 * kt
  perf:
  while kcnt < 2000 do  
    kbar dict_get idict, "bar"
    kfoo dict_get idict, "foo"
    kcnt += 1
  od
endin

    
instr 13
  ; tests dict_set performance
  idict dict_new "sf"
  kt timeinstk
  ; printk2 kt

  kcnt = 0
  while kcnt < 2000 do
    Sbar = sprintfk("bar%d", kcnt+kt*10000)
    dict_set idict, Sbar, kcnt
    ;kout dict_get idict, Sbar
    ;printf "key: %s  value: %f\n", kcnt, Sbar, kout
    ;printk2 kout
    kcnt += 1
  od
endin

instr 23
  ; tests chnset performance
  kt timeinstk
  ; printk2 kt

  kcnt = 0
  while kcnt < 2000 do
    Sbar = sprintfk("bar%d", kcnt+kt*10000)
    chnset kcnt, Sbar
    ;kout chnget Sbar
    ;printf "key: %s  value: %f\n", kcnt, Sbar, kout
    
    kcnt += 1
  od
endin

    
; test S -> f
instr 20
  kt timeinstk
  printk2 kt

  kcnt = 0
  kval = 0
  while kcnt < 2000 do
    ; kval = kt*10000 + kcnt
    chnset kt*10000 + kcnt, "bar"
    chnset kt*10000 + kcnt, "foo"
    
    kbar chnget "bar"
    kfoo chnget "foo"
    ; printf "bar: %f   foo: %f\n", kval, kbar, kfoo
    
      kcnt += 1
    od
  
endin

; compare khash to chnget/set
; test S -> f
instr 21
  kt timeinstk
  printk2 kt

  kcnt = 0
  kbar init 0
  kfoo init 0
  
  while kcnt < 2000 do
    Skey1 = sprintfk("bar%d", kcnt)
    Skey2 = sprintfk("foo%d", kcnt)
    chnset kt*10000+kcnt, Skey1
    chnset kt*10000+kcnt*2, Skey2
    kbar chnget Skey1
    kfoo chnget Skey2

    ; printf "bar: %f   foo: %f\n", kcnt, kbar, kfoo
      
      kcnt += 1
    od
  
endin
    
instr 30
  ; test dict free
  idict1 dict_new "sf"
  print idict1
  idict2 dict_new "sf", 1
  print idict2
  dict_free idict2, 1
endin

instr 40
  ; test del
  idict dict_new "ss"
  kt timeinstk
  Skey sprintfk "key%d", kt
  dict_set idict, Skey, sprintfk("val%d", kt)
  Sval dict_get idict, Skey
  printf "key: %s  val: %s\n", kt, Skey, Sval
endin

</CsInstruments>
<CsScore>
; i 1 0 0.01
; i 10 0.1 1
i 13 0 5
</CsScore>
</CsoundSynthesizer>
