# dict_iter

## Abstract

Iterate over the key-value pairs of a dict 

## Description

Iterates over the key:value pairs. Whenever kreset is 1, iteration starts over
If kreset is -1 (the default), iteration is autotriggered when it reaches
the end of the collection.
 
 
!!! note

    `dict_iter` is meant to be used __in a loop__ at **k-time**

## Syntax

    xkey, xvalue, kidx  dict_iter idict [, kreset = -1]

### Arguments

* `idict`: the handle to the dict as returned by [dict_new](dict_new.md)
* `kreset`: the reset policy


| kreset      | effect                                                                                    |
| ----------- | ----------------------------------------------------------------------------------------- |
| 0           | no reset, iteration stops at the end of the collection. There will be at most 1 iteration |
| 1 (default) | Iteration starts over at every k-cycle                                                    |
| 2           | Reset at the end of iteration (independent of k-cycle)                                    |

### Output

* `xkey` / `xvalue`: the key and value for this pair. The types are determined by the type of
  this dict, as defined via `dict_new`
* `kidx`: the index of this pair. It will be 0 for the first pair, 1 for the second, etc.
  When no more pairs, `kidx` will be -1. In this case, xkey and xvalue do not hold valid values
 

### Execution time

`dict_loop` executes only at **Performance Time**. 


## Usage

There are two ways to use `dict_iter`, either in a `while` loop, or using gotos

```csound

; iterate in a while loop
kidx = 0
while kidx < dict_size(idict)-1 do
    Skey, kvalue, kidx dict_iter idict
    printf "key: %s  value: %f", kidx, Skey, kvalue
od

; iterate with goto
loop:
    Skey, kvalue, kidx dict_iter idict
    if kidx == -1 goto break
    printf "key: %s  value: %f", kidx, Skey, kvalue
    kgoto loop
break:
    
```

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
--nosound 
</CsOptions>
<CsInstruments>

/*

  # Example file for dict_iter opcode 

*/
  
instr 1
  /*
  
  dict_iter
  
  xkey, xvalue, kidx dict_iter ihandle [,kreset=1]
  
  kidx: holds the number of pairs yielded since last reset. It 
        is set to -1 when iteration has stopped 
        (in this case, xkey and xvalue are invalid and should not
        be used)
  kreset = 0  -> after iterating over all pairs iteration stops
                 In this mode, iteration happens at most once
           1  -> iteration starts over every k-cycle
           2  -> iteration restarts after stopping   
  */
  
  idict dict_new "sf", 0, "foo", 1, "bar", 2, "baz", 15, "bee", 9

  ; iterate with a while loop
  kidx = 0
  while kidx < dict_size(idict) - 1 do 
    Skey, kvalue, kidx dict_iter idict 
    printf "while) %s -> %f \n", kidx+kt*1000, Skey, kvalue
  od   

  ; the same but with goto
loop:
  Skey, kvalue, kidx dict_iter idict
  if kidx == -1 goto break
  printf "loop) %s -> %f \n", kidx+kt*1000, Skey, kvalue
  kgoto loop
break:

endin


</CsInstruments>
<CsScore>
i 3 0 0.05

</CsScore>
</CsoundSynthesizer>


```


## See also

* [dict_new](dict_new.md)
* [dict_size](dict_size.md)


## Credits

Eduardo Moguillansky, 2019
