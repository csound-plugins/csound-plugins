# dict_query

## Abstract

Query different properties of a dict

## Description

Properties of a dict which do not need a separate opcode (because they are not used very frequently) are merged togehter in this opcode. 
Possible properties are:

* `exists`: return 1 if the handle points to a valid dict
* `size`: returns the size of this dict (similar to dict_size)
* `type`: returns an integer identifying the type of this dict
  * `11`: number -> number
  * `12`: number -> string
  * `21`: string -> number
  * `22`: string -> string
* `keys`: returns an array with all the keys defined in this dict (the type of the returned array depends on the type of the keys)
* `values`: returns an array with all the values (the type of the returned array depends on the type of the values). 

**NB**: the order in which keys and values are returned represents the corresponding pairs, so keys[i] and values[i] belong to the same pair.

`dict_query` executes at **k-time**. 


## Syntax

    kexists   dict_query idict, "exists"
    ksize     dict_query idict, "size"
    ktype     dict_query idict, "type"
    SKeys[]   dict_query idict, "keys"
    kKeys[]   dict_query idict, "keys"
    kValues[] dict_query idict, "values"
    SValues[] dict_query idict, "values"
    
## Arguments

* `idict`: the handle of the dict, as returned by `dict_new`

## Output

**NB**: the arrays returned from keys and values can have either an S- or a k-type, 
depending on the type definition of the array 

## Execution Time

* Performance

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
--nosound 
</CsOptions>
<CsInstruments>

/*

  Example file for dict_query

  ... dict_query idict, Scmd

  where Scmd can be: 'size', 'type', 'exists', 'keys', 'values'

*/


instr 1
  ; get all the keys as an array
  idict1 dict_new "sf", 0, "keyA", 1, "keyB", 2, "keyC", 3
  ; check that the dict exists
  kexists dict_query idict1, "exists"
  ktype   dict_query idict1, "type"
  printf "dict exists = %d, type = %d \n", 1, kexists, ktype
  
  SKeys[] dict_query idict1, "keys"
  printarray SKeys

  ; now check a bogus handle
  kexists999 dict_query 999, "exists"
  printf "dict 999 exists = %d \n", 1, kexists999
  
  ; get the keys from a different dict
  idict2 dict_new "if", 0, 1,100, 10,1000, 2,200
  kKeys[] dict_query idict2, "keys"
  printarray kKeys, 1, "%.0f"
  printf "idict2 has size=%d\n", 1, dict_query(idict2, "size")
    
  ; get values as an array
  idict3 dict_new "is", 0, 10, "foo", 20, "bar", 30, "baz"
  Svals[] dict_query idict3, "values"
  printarray Svals
    

  kVals[] dict_query idict2, "values"
  printarray kVals

  ; query the type of the dict
  ktype  dict_query idict2, "type"
  printf "the type of dict %d is %d\n", 1, idict2, ktype  

  turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>


```

## See also

* [dict_iter](dict_iter.md)
* [dict_size](dict_size.md)

## Credits

Eduardo Moguillansky, 2019
