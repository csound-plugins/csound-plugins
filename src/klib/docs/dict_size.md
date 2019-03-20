# dict_size

## Abstract

Returns the number of key:value pairs in a dict

## Description

A hashtable is a mapping from a key to a value. The `dict_` family of opcodes 
implement a hashtable mapping either strings or integers to strings or floats. 

`dict_size` returns the number of such key:value pairs inside a dict. If the handle
passed does not point to a valid dict, `dict_size` returns -1. It is thus possible
to use it to check that the passed handle is valid (similar to `dict_query idict "exists"`,
see [dict_query](dict_query.md))

`dict_size` can be used together with [dict_iter](dict_iter) to iterate over the key:value
pairs 

## Syntax

    ksize dict_size idict
    
`dict_size` executes both at **i-time** and **k-time**. 

## Arguments

* `ìdict`: the handle of the dict, as returned by `dict_new`

### Output

* `ksize`: the number of key:value pairs in the dict


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
  idict dict_new "sf"
    
  ; set key a key:value pair
  dict_set idict, "bar", 123
  
  ksize = dict_size(idict)
  printf "size: %d", 1, ksize
  
  dict_set idict, "foo", 4
  ksize = dict_size(idict)
  printf "size: %d", 1, ksize
  
  ; size does not change now
  dict_set idict, "foo", 10
  ksize = dict_size(idict)
  printf "size: %d", 1, ksize
  
  ; we delete a key, size is reduced
  dict_set idict, "foo"
  ksize = dict_size(idict)
  printf "size: %d", 1, ksize
  
  turnoff
endin


</CsInstruments>
<CsScore>

i 1 0 1

</CsScore>
</CsoundSynthesizer> 
```

## See also

* [dict_iter](dict_iter.md)
* [dict_set](dict_set.md)

## Credits

Eduardo Moguillansky, 2019
