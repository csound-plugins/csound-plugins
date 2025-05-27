# dict_delk

## Abstract

Remove a key:value pair from a hashtable at perf time

## Description

Copy of `dict_del` for the case of string keys, deletes the key at performance
(dict_del will delete the key:value pair at init time for string keys).

A hashtable is a mapping from a key to a value. The `dict_` family of opcodes
implement a hashtable mapping either strings or integers to strings or floats.
`dict_del` removes a key:value pair from the hashtable. If both dict and key are
i- variables, `dict_del` executes only at i-time. Otherwise it executes at every
k- cycle. **Use an `if` guard to prevent this, if so needed**. To delete a key at the
end of an event, use [defer](defer.md)

## Syntax

    dict_del idict, Skey      ; execution at k- time
        

## Arguments

* `idict`: the handle of the dict, as returned by `dict_new`
* `Skey`: the key to remove


## Execution Time

* Init
* Performance

## Example

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

  dict_print idict
  
  dict_delk idict, "bar"
  
  dict_print idict, -1

  turnoff
endin


</CsInstruments>
<CsScore>

i 1 0 2

</CsScore>
</CsoundSynthesizer>
```

## See also

* [dict_del](dict_del.md)
* [dict_new](dict_new.md)
* [dict_get](dict_get.md)
* [dict_set](dict_set.md)
* [defer](defer.md)
* [dict_free](dict_free.md)

## Credits

Eduardo Moguillansky, 2019
Last update: 2021
