# dict_print

## Abstract

Prints the contents of a dict

## Description

`dict_print` can be used to print the contents of a dict, mostly for debugging
purposes

`dict_print` can print both at **i-time** or **k-time**

## Syntax

    dict_print idict [, ktrig=1]
    
## Arguments

* `ìdict`: the handle of the dict, as returned by `dict_new`
* `ktrig`: when to print the dict. Printing will take place whenever ktrig is -1 or 
           if ktrig is a positive and higher than the last trig

## Examples

```csound

<CsoundSynthesizer>
<CsOptions>

--nosound

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1	
  ; create a local dict, mapping strings to numbers
  idict dict_new "sf", "foo", 10, "bar", 20, "baz", 30
  print_dict idict, 1

  ; print with a metro
  idict2 dict_new "ss", "foo", "foofoo", "bar", "barbar", "baz", "bazbaz"

  ktrig metro 2
  print_dict idict2, ktrig
  
  turnoff
endin


</CsInstruments>
<CsScore>

i 1 0 1

</CsScore>
</CsoundSynthesizer> 
```

## See also

* [dict_iter](dict_iter)
* [dict_set](dict_set)
* [dict_query](dict_query)

## Credits

Eduardo Moguillansky, 2019
