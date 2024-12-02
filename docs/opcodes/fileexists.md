# file_exists

## Abstract

Returns 1 if a file exists and can be read

## Description

`file_exists` checks if a given file path exists and returns 1 if it does, 0 otherwise.
It doesn't check any search path of csound and does not expand any variables 
(like "~" in unix). 

## Syntax

```csound

iexists  fileexists  Spath

```
    
### Arguments

* `Spath`: the path to check

### Output

* `iexists`: 1 if the file exists, 0 otherwise

### Execution Time

* Init

## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
   
</CsOptions>

<CsInstruments>

/*
    Example file for file_exists

    file_exists returns 1 if a given path refers to an existing file
   
*/

ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
	iexists fileexists "file_exists.csd"
    print iexists
    turnoff
endin

</CsInstruments>

<CsScore>
i1 0 1

</CsScore>
</CsoundSynthesizer>



```


## See also

* [filevalid](https://csound.com/docs/manual/filevalid.html)
* [system](https://csound.com/docs/manual/system.html)

## Credits

Eduardo Moguillansky, 2019
