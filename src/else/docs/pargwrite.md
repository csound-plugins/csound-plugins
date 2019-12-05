# pargwrite

## Abstract

Modify parg values of an active instrument instance

## Description

`pargwrite` can be used to modify a running instance (possibly a 
fractional instrument number) of an instrument,
modifying parg values also after the instance has been started.
It no instance is found that matches the given instrument number,
a warning will be printed to the console and this opcode will fail
silently.
If the instance ceases to exist during another instrument is modifying
its parg values, nothing happens. 
At the moment there is no bounds checking regarding the parg index. 
Writing to a parg which has not been set as the instance was scheduled
(either in the score or via the multiple scheduling opcodes, like "event",
"schedule", etc) will result in *undefined behaviour*.

## Syntax

    pargwrite instrnum, i/kindex, i/kvalue
    
### Arguments

* `instrnum`: the (fractional) instrument number to modify
* `iindex` / `kindex`: the index of the parg to modify. If kindex is 4, then p4 will be 
  modified
* `ivalue` / `kvalue`: the new value of the given parg

### Output

### Execution Time

* Init (if index and value are i-values)
* Performance (if either index or value are k-variables)

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>

--nosound
-m0

</CsOptions>

<CsInstruments>

/*
    Example file for pargread

    ivalue pargread instrnum, indx [, inotfound=-1]

    pargread reads a parg value from an active instrument
    Returns inotfound if instrnum is not active
   
*/

instr 1
    k4 = p4
    printf "instance: %.3f, p4: %f \n", metro(10), p1, k4
endin

instr 2
    kval line 10, p3, 20
    pargwrite 1.01, 4, kval
endin

</CsInstruments>

<CsScore>
i 1.01 0 2 101
i 1.02 0 2 102
i 2 1 0.5

</CsScore>
</CsoundSynthesizer>


```


## See also

* [pargread](pargread.md)
* [pset](https://csound.com/docs/manual/pset.html)
* [p](https://csound.com/docs/manual/p.html))
* [passign](https://csound.com/docs/manual/passign.html)

## Credits

Eduardo Moguillansky, 2019
