# dict_new

## Abstract

Create a hashtable 

## Description

A hashtable is a mapping from a key to a value. The `dict_` family of opcodes 
implement a hashtable mapping either strings or integers to strings or floats. 
A hashtable can be either local, in which case its lifespan is fixed to the lifespan 
of the note it was created in; or it can be global, in which case it is not 
deallocated when the note is released and is kept alive until either the end 
of the performance, or until freed via `dict_free`

`dict_new` runs only at **i-time**

## Syntax

    idict dict_new Stype [, isglobal=0]
    idict dict_new Stype, isglobal, key0, value0, key1, value1, ...

`dict_new` executes only at **init time**. 
    
**NB**: With the second variant it is possible to create a dict and give it initial values at init-time. 

## Arguments

* `Stype`: a string describing the type of the key and the value. Possible values are:
    * "sf" or "str:float": string → float
    * "ss" or "str:str": string → string
    * "is" or "ìnt:str": int → string
    * "if" or "int:float": int → float
     
* `isglobal`: if 1, the dict will outlive the instrument it was created in and 
              will stay active until either the end of the performance or if 
              destroyed via `dict_free`. Default is 0 (local)
* `key0`, `value0`, etc: initial pairs can be set at creation time, matching 
              the types declared with `Stype` 

### Output

* `idict`: identifies this dict with an integer. This integer can be passed around to another instrument and will always resolve to the same dict, as long as this dict is still alive. This can be checked via `dict_query idict, "exists"`

### Execution Time

* Init


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

* [dict_free](dict_free.md)
* [dict_set](dict_set.md)

## Credits
