# strmul

## Abstract

Produce multiple copies of a string

## Description

Concatenates a string any number of times

## Syntax


```csound

Sout strmul Sin, itimes, imaxlen=1024
Sout strmul Sin, ktimes, imaxlen=1024

```
    
## Arguments

* **Sin**: The source string
* **itimes**: The number of times to duplicate the string. 
* **ktimes**: The number of times to duplicate the string.
* **imaxlen**: Maximum length of the resulting string. This needs to be known at init time
  to avoid allocations at performance

## Output

* **Sout**: The resulting string

## Execution Time

* Init (if itimes is an init variable)
* Performance (if ktimes is a k-variable)

## Examples


```csound


<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

instr 1
	Sout = strmul("*", 10)
	println "Sout: %s", Sout
	turnoff
endin

instr 2
	ksig = oscil:k(0.5, 5)+0.5
	Ssig = strmul("*", round(ksig*80));
	printsk "ksig: %s\n", Ssig
endin
</CsInstruments>

<CsScore>

i 2 0 1
</CsScore>
</CsoundSynthesizer>



```


## See also

* [strjoin](strjoin.md)
* [strsplit](strsplit.md)


## Credits

Eduardo Moguillansky, 2025


## Metadata

* Author: Eduardo Moguillansky
* Year: 2025
* Plugin: else
* Source: https://github.com/csound-plugins/csound-plugins/blob/master/src/else/src/else.c
