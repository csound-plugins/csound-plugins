# dict_free

## Abstract

Free a hashtable 

## Description

frees the hashtable either at init time or at the end of the note (similar to ftfree)
A dict can only be freed if it was created as global (see [dict_new](dict_new.md)).
To be able to pass a dict between notes, a note can create a global dict and 
pass its handle to another note. When the first note is released, the dict lives
on (because it is global), and it will be either freed at the end of the performance
or explicitely by calling `dict_free`

## Syntax

    dict_free idict [, iwhen=0] 

`dict_free` executes only at **init time**. 
    
## Arguments

* `idict`: the handle of the dict to be freed
* `iwhen`: similar to `ftfree`
    * if `iwhen == 0` : free the dict now
    * if `iwhen == 1` : free the dict at the end of this note

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

[dict_new](dict_new.md)

## Credits

Eduardo Moguillansky, 2019