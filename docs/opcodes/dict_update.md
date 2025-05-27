# dict_update

## Abstract

Update a dict with another dict

## Description

Updates a base dict with the key-value pairs of another dict. Any key value
pair from the second dict which not present in the base dict
is added to the base dict. The second dict is not modified


## Syntax

    dict_update ibasedict, iupdatedict

## Arguments

* `ibasedict`: the base dict, as returned by `dict_new`
* `iupdatedict`: the dict with the updates


## Execution Time

* Init

## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
-m0
--nosound
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
	idict1 dict_new "sf", "foo", 1, "bar", 2
	idict2 dict_new "sf", "bar", 20, "baz", 30
	dict_update idict1, idict2
	dict_print idict1
	turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1

</CsScore>
</CsoundSynthesizer>



```

## See also

* [dict_new](dict_new.md)
* [dict_get](dict_get.md)
* [dict_del](dict_del.md)
* [dict_free](dict_free.md)

## Credits

Eduardo Moguillansky, 2025
