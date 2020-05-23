# dict_geti

## Abstract

Get a string value from a hashtable at init time

## Description

For dicts of type str:str or str:any, this version of dict_get runs at
init-time only. See [dict_get](dict_get.md) for any further details

## Syntax

    Svalue dict_geti idict, Skey 

## Arguments

* `ìdict`: the handle of the dict, as returned by `dict_new`
* `Skey` / `kkey`: the key to be queries, as previously set by [dict_set](dict_set)

### Output

* `Svalue`: the value corresponding to the key

### Execution Time

* Init

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>
<CsInstruments>

/*

  # Example file for dict_geti

  ## dict_geti

    Svalue dict_geti idict, Skey

    
  Get the value at a given key at init time. An empty string
  is returned when the key is not found.
    
*/

ksmps = 64
nchnls = 2
0dbfs = 1

instr 1
  ; create a local dict, mapping strings to numbers
  idict dict_new "sa", "foo", "foovalue", "bar", 10

  Sfoo dict_geti idict, "foo"
  prints "Soo: %s \n", Sfoo
  
  turnoff
endin


</CsInstruments>
<CsScore>
i 1 0 0.01
f 0 1
</CsScore>
</CsoundSynthesizer>

```

## See also

* [dict_new](dict_new.md)
* [dict_set](dict_set.md)
* [dict_get](dict_get.md)


## Credits

Eduardo Moguillansky, 2020