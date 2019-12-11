# dict_exists

## Abstract

Returns 1 if the dict exists, 0 otherwise

## Description

`dict_exists` checks if the index passed refers to an existing dict and returns
1 if it does, 0 otherwise. 

**It works at i-time only**

## Syntax

    iexists dict_exists idict
    

### Arguments

* `ìdict`: the handle of the dict, as returned by `dict_new`

### Output

* `iexists`: 1 of the dict exists, 0 otherwise


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

  Example file for dict_exists

  iexists dict_exists idict
  
*/

opcode accum, k, ko
  kstep, ival xin
  know init ival
  xout know
  know += kstep
endop

instr 1
  idict = p4
  print idict
  if(dict_exists(idict)==1) then
    kfoo dict_get idict, "foo"
    kbar dict_get idict, "bar"
    ; dict_free idict, 1
  else
    kfoo init 10
    kbar init 20
  endif

  printf "$$$ foo=%f  bar=%f \n", accum(changed(kfoo, kbar)), kfoo, kbar
endin

instr 2
  idict dict_new "*str:float", "foo", 1, "bar", 2
  schedule 1, 0, p3, idict
  dict_set idict, "foo", linseg:k(1, p3, 2)
  dict_set idict, "bar", linseg:k(2, p3, 3)
endin

instr 3
  schedule 1, 0, p3, -1
endin



</CsInstruments>
<CsScore>
i 2 0 1
i 3 4 4
</CsScore>
</CsoundSynthesizer>


```


## See also

* [dict_iter](dict_iter.md)
* [dict_size](dict_set.md)
* [dict_query](dict_query.md)


## Credits

Eduardo Moguillansky, 2019
