# dict_set

## Abstract

Set a value from a hashtable

## Description

A hashtable is a mapping from a key to a value. The `dict_` family of opcodes
implement a hashtable mapping either strings or integers to strings or floats.
`dict_set` sets the value corresponding to a key if the key is already present,
or inserts a key:value pair otherwise.

**NB**: To remove a key-value pair use `dict_del`

!!! Tip

    It is possible to set multiple values at i-time directly with [dict_new](dict_new.md)

## Syntax

    dict_set idict, xkey, xvalue

## Arguments

* `idict`: the handle of the dict, as returned by `dict_new`
* `xkey`: the key to set. Its type must match the type definition of the dict.
          (a string or a possitive integer)
* `xvalue`: the value to set. Its type must match the type definition of the
  dict (a str or a numeric value)


## Execution Time

* Init
* Performance

## Examples

```csound

<CsoundSynthesizer>
<CsOptions>
; -odac  -iadc    ;;;RT audio out and in

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1
  ; create a local dict, mapping strings to numbers
  idict dict_new "sf"
  dict_free idict
  
  ; set key a key:value pair
  dict_set idict, "bar", 123

  ; retrieve the value
  kbar dict_get idict, "bar"

  ; get a non-existent key, will output the default
  kfoo dict_get idict, "foo", -1

  printf ">>>> bar: %f,  foo: %f \n", 1, kbar, kfoo

  ; now create another dict, this one will outlive this note
  idict2 dict_new "str:str", "baz", "bazvalue", "foo", "foovalue"

  ; schedule another inst, pass this dict
  event "i", 2, 0, 1, idict2
  turnoff
endin

instr 2
  idict = p4
  print_dict idict

  Sbaz = dict_get(idict, "baz")
  printf "instr 2, kbaz = %s \n", 1, Sbaz

  ; free dict at the end of this note
  dict_free idict, 1
  turnoff
endin

; schedule 1, 0, 1

</CsInstruments>
<CsScore>

i 1 0 2

</CsScore>
</CsoundSynthesizer>
```

```csound


<CsoundSynthesizer>
<CsOptions>
-m0
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

gidict dict_new "sf"

opcode argset, 0, iSk
  ip1, Sparam, kvalue xin
  Skey sprintf "%.4f:%s", ip1, Sparam
  dict_set gidict, Skey, kvalue
endop

opcode argget, k, Si
  Sparam, idefault xin
  Skey sprintf "%.4f:%s", p1, Sparam
  kout dict_get gidict, Skey, idefault
  xout kout
  ;; delete key at end of event
  defer "dict_set", gidict, Sparam 
endop

instr exit
  prints "Exiting csound \n"
  exitnow
endin

instr 1
  inum uniqinstance 2
  kfreq = linseg(random(4000, 2000), p3, random(300, 350))
  argset inum, "freq", kfreq
  schedule inum, 0, 3
  ksize dict_size gidict
  printk2 ksize
  atstop p1, 0, p3
endin

instr 2
  kfreq argget "freq", 1000 
  a0 oscili 0.02, kfreq
  a0 *= linsegr:a(0, 0.1, 1, 0.1, 0)
  outs a0, a0
endin

instr example_dict
  schedule 1, 0, 0.01
  schedule "exit", 10, 0.1
  turnoff
endin

;; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

instr 10
  inum uniqinstance 11, 10000
  kfreq = linseg(4000, p3, random(300, 350))
  Schan = sprintf("%f_freq", inum)
  dict_set gidict, Schan, kfreq
  schedule inum, 0, p3
endin

instr 11
  Skey sprintf "%f_freq", p1
  printf "p1=%.6f \n", 1,  p1
  kfreq dict_get gidict, Skey, 1000
  ;; delete key at end of event
  defer "dict_set", gidict, Skey
  a0 oscili 0.02, kfreq
  a0 *= linsegr:a(0, p3*0.1, 1, p3*0.9, 0)
  outs a0, a0
endin

instr dictsize
  isize = dict_size:i(gidict)
  print isize
  turnoff
endin

instr example2
  i0 = 0
  istep = 0.01
  idur = 0.3
  while i0 < 10000 do 
    schedule 10, i0 * istep, idur
    i0 += 1
    print i0
  od
  iendtime = (i0 + 2) * istep + idur
  schedule "dictsize", iendtime-0.005, -1
  schedule "exit", iendtime, -1
  turnoff
endin

;; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

instr 20
  inum uniqinstance 21, 10000
  kfreq = linseg(4000, p3, random(300, 350))
  Schan = sprintf("%f_freq", inum)
  ; printf "%s \n", 1, Schan
  chnset kfreq, Schan
  schedule inum, 0, p3
endin

instr 21
  Skey sprintf "%f_freq", p1
  printf "p1=%.6f \n", 1,  p1
  kfreq chnget Skey
  a0 oscili 0.02, kfreq
  a0 *= linsegr:a(0, p3*0.1, 1, p3*0.9, 0)
  outs a0, a0
endin

instr example3
  ; the same as example2 but with channels
  i0 = 0
  istep = 0.01
  idur = 0.3
  while i0 < 10000 do 
    schedule 20, i0 * istep, idur
    i0 += 1
    print i0

  od
  schedule "exit", (i0 + 1) * istep + idur, -1
  turnoff
endin

; schedule "example_dict", 0, 1
schedule "example2", 0, 1
; schedule "example3", 0, 1


</CsInstruments>
e 10
<CsScore>

</CsScore>
</CsoundSynthesizer>


```

## See also

* [dict_new](dict_new.md)
* [dict_get](dict_get.md)
* [dict_del](dict_del.md)
* [defer](defer.md)
* [dict_free](dict_free.md)

## Credits

Eduardo Moguillansky, 2019
Last update: 2021
