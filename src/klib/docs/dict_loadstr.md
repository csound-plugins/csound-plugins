# dict_loadstr

## Abstract

Creates and fills a new dict from a string definition


## Description

`dict_loadstr` creates a new dict and fills it from a string
definition. Here is an example of such a string definition:


    "keyA: 10, keyB: string value, keyC: 'quoted string'"


The dict created has the type `str:any`. The keys are always strings
and the values can be either numbers or strings. Keys do not need to
be quoted. Values only need to be quoted if a string value consists of
only numbers or includes itself quotation marks. Quotations must be
single quotations: `'`

The motivation behind this opcode is to be able to save a dictionary
to a string, for example in the metadata of a soundfile, and be
able to retrieve that as a dictionary inside csound
  
## Syntax

    idict dict_loadstr Sdefinition
    
## Arguments

* **Sdefinition**: the string defining the key:value pairs
* **idict**: the dict handle


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
  # Example file for dict_loadstr

  ## dict_loadstr

    idict dict_loadstr Sdictdef

  Create a new dict from a string definition of the form
  "keyA: valueA, keyB: valueB, ..."

  The dict creates has the type str:any, which means that the 
  keys are always strings and the values can be either numbers
  or strings. Keys do not need to be quoted. Values only need to
  be quoted if a string value consists of only numbers or includes
  itself quotation marks. Quotations must be single quotations: `'`

  The motivation behind this opcode is to be able to save a dictionary
  to a string, for example in the metadata of a soundfile, and be
  able to retrieve that as a dictionary inside csound
  
*/

instr example1
  idict dict_loadstr "foo: 10,  bar :'barvalue', \nbaz: unquoted string"
  dict_print idict
  turnoff
endin


</CsInstruments>

<CsScore>
i "example1" 0 0.1
</CsScore>
</CsoundSynthesizer>


```

## See also

* [dict_dump](dict_dump.md)
* [dict_new](dict_iter.md)
* [dict_set](dict_set.md)
* [sfreadmeta](sfreadmeta.md)

## Credits

Eduardo Moguillansky, 2021
