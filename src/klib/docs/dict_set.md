# dict_get

## Abstract

Set (or remove) a value from a hashtable

## Description

A hashtable is a mapping from a key to a value. The `dict_` family of opcodes 
implement a hashtable mapping either strings or integers to strings or floats. 
`dict_set` sets the value corresponding to a key if the key is already present,
or inserts a key:value pair otherwise. Without a value it deletes the key:value
pair from the dict.

## Syntax

    dict_set idict, xkey, xvalue
    dict_set idict, xkey

If xvalue is not given, `dict_set` removes the key from the dict    
  
`dict_get` executes both at **i-time** and **k-time**. 

**NB**: it is possible to set multiple values at i-time directly with [dict_new](dict_new)

## Arguments

* `idict`: the handle of the dict, as returned by `dict_new`
* `xkey`: the key to set. Its type must match the type definition of the dict.
          (a string or a possitive integer)
* `xvalue`: the value to set. Its type must match the type definition of the dict (a str or a numeric value)


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
  
  ; set key a key:value pair
  dict_set idict, "bar", 123

  ; retrieve the value
  kbar dict_get idict, "bar"
  
  ; get a non-existent key, will output the default
  kfoo dict_get idict, "foo", -1 

  printf ">>>> bar: %f,  foo: %f \n", 1, kbar, kfoo 

  ; now create another dict, this one will outlive this note
  idict2 dict_new "ss", 1, "baz", "bazvalue", "foo", "foovalue"
  
  ; schedule another inst, pass this dict
  event "i", 2, 0, 1, idict2
  
  turnoff

endin

instr 2
  idict = p4
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

## See also

* [dict_new](dict_new)
* [dict_get](dict_get)

## Credits

Eduardo Moguillansky, 2019