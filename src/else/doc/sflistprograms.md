# sflistprograms

## Abstract

List the available programs in a soundfont without loading the samples


## Description

This opcode is very similar to `fluidInfo`. Given the path to a
soundfont, it returns an array of strings, where each string has the
form `<bank>-<programnumber> <programname>` where `bank` is the bank
number, `programnumber` is the program number (or preset number, both
names are often used), as passed to
[fluidProgramSelect](http://www.csound.com/docs/manual/fluidProgramSelect.html)
or [sfinstr](http://www.csound.com/docs/manual/sfinstr.html). 

## Syntax


```csound
Sprograms[] sflistprograms Spath

```
    
## Arguments

* **Spath**: the path to a soundfont (.sf2) file

## Output

* **Sprograms**: the list of available programs

## Execution Time

* Init 

## Examples


```csound


<CsoundSynthesizer>
<CsOptions>

--nosound
; -m0

</CsOptions>

<CsInstruments>


instr 1
  Sprograms[] sflistprograms "violin.sf3"
  printarray Sprograms
  turnoff
endin

/*
Prints:

"000-000 Campbells Violin", 
"000-001 Campbells V Loop", 
"000-002 Cam's Violin Reverb"
"000-003 Cam's Violin Panned", 
"000-004 Violin- Pan & Reverb", 
"000-005 Cams Violin- tinny"
"000-006 Cams Violin-Vibrato"

*/

</CsInstruments>

<CsScore>

i 1 0 0.1

</CsScore>
</CsoundSynthesizer>



```


## See also

* [fluidInfo](http://www.csound.com/docs/manual/fluidInfo.html)
* [sfinstr](http://www.csound.com/docs/manual/sfinstr.html)

## Credits

Eduardo Moguillansky, 2021
