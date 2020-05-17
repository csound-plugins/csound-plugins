# pread

## Abstract

Read pfield values from any active instrument instance

## Description

`pread` can be used to query the pfield value of a running instance (possibly a 
fractional instrument number) of an instrument. Together with 
`pwrite` it can be used to establish a two-way communication
between two running instances of any two instruments.

It no instance is found that matches the given instrument number,
the output value is set to `inotfound`. 

### Behaviour

* If no matching instance is found, pread returns `inotfound`. Reporting
  starts at the moment a matching instance is found. 
* To avoid a continuous search, set `instrnum` as negative number. In this case,
  a matching instance is searched only once, and, if not found, this opcode 
  becomes a `noop`
* If an instance is found and stops, pread returns `inotfound` from
  the moment the instance stops and no new instance is searched. 
  
## Syntax

```csound
output:i|k pread instrnum:i, index:i|k [, inotfound=-1] 
```
    
### Arguments

* `instrnum` (i):  the (fractional) instrument number to modify
* `index` (i or k): the index of the pfield to read. If kindex is 4, then p4 will be 
  modified
* `inotfound`: the value to return if instrnum is not found. To avoid misinterpretation,
  this value should be different than any expected value of the pfield

### Output

* `output` (i or k): the current value of the given pfield. Will be `inotfound` if
  no matching instance has been found.

### Execution Time

* Init (if output is of i-type)
* Performance (if output is of k-type)

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>

--nosound
-m0

</CsOptions>

<CsInstruments>

/*

  Example file for pread
  ======================

  ivalue pread instrnum, index  [, inotfound=-1]
  kvalue pread instrnum, kindex [, inotfound=-1]

  pread reads a pfield value from an active instrument
  Returns inotfound if instrnum is not active

  Raises a performance error if index is out of range

*/

instr 1
  print p4
endin

instr 2
  ip4 pread 1.01, 4
  printf "<<<< instr 1.01 p4=%f >>>>\n", 1, ip4
  turnoff
endin

</CsInstruments>

<CsScore>
i 1.01 0 2   95
i 2    1 0.1

</CsScore>
</CsoundSynthesizer>


```


## See also

* [pwrite](pwrite.md)
* [pset](https://csound.com/docs/manual/pset.html)
* [p](https://csound.com/docs/manual/p.html)
* [passign](https://csound.com/docs/manual/passign.html)
* [uniqinstance](uniqinstance.md)

## Credits

Eduardo Moguillansky, 2019
