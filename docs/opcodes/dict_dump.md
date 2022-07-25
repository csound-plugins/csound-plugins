# dict_dump

## Abstract

Dumps the contents of this dict as a string


## Description

`dict_dump` serializes the contents of a dict to a string which can be
used to reconstruct such dict. Here is an example of such a string
definition:

    "keyA: 10, keyB: string value, keyC: 'quoted string'"

This string can be used to load the contents of a dict via 
[dict_loadstr](dict_loadstr.md). At the moment only dicts with string keys
can be dumped.

The motivation behind this opcode is to be able to save a dict to a
string, for example in a textfile or in the metadata of a soundfile,
and be able to retrieve that as a dict later, in csound or in anothe
software.
  
  
## Syntax

    Sdump dict_dump idict
    
## Arguments

* **idict**: the dict handle
* **Sdump**: the string defining the key:value pairs


## Execution Time

* Init

## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

/*
  # Example file for dict_dump

  ## dict_dump

    Sdump dict_dump idict

  Dump the contents of the dict to a string, in the following format:

    "keyA: valueA, keyB: valueB, ..."

  This string can be used to load the contents of a dict via 
  dict_loadstr. At the moment only dicts with string keys
  can be dumped.

  The motivation behind this opcode is to be able to save a dict
  to a string, for example in a textfile or in the metadata of a 
  soundfile, and be able to retrieve that as a dict later, in
  csound or in anothe software.
  
*/

instr example1
  ; Uncomment to test with different dict types
  ; idict dict_new "str:float", "foo", 10.1, "bar", 0.5, "baz", 0.12345678
  idict dict_new "str:any", "foo", 10.1, "baz", 0.12345678, "bar", "barvalue" 
  ; idict dict_new "str:str", "foo", "fooval", "bar", "barval"

  Sdump dict_dump idict
  prints "\ndump: <\n%s\n>\n\n", Sdump

  prints "\n--- idict ---\n"
  dict_print idict
    
  idict2 dict_loadstr Sdump
  prints "\n--- idict2 ---\n"
  dict_print idict2
  prints "\n"
  turnoff
endin


</CsInstruments>

<CsScore>
i "example1" 0 0.1
</CsScore>
</CsoundSynthesizer>


```

## See also

* [dict_loadstr](dict_dump.md)
* [dict_new](dict_iter.md)
* [dict_set](dict_set.md)
* [filereadmeta](filereadmeta.md)

## Credits

Eduardo Moguillansky, 2021
