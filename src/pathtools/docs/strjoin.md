# strjoin

## Abstract

Concatenate any number of strings


## Description

Join multiple strings, either as an array or as arguments. The separator
is inserted in between each given strings. The results is returned
as a new strings

## Syntax

    Sout strjoin Ssep, Sstrings[]
    Sout strjoin Ssep, Sstr1, Sstr2, ..., Sstrn

## Arguments

* `Ssep`: a string to be inserted between each given string
* `Sstrings`: an array of strings

## Output

* `Sout`: the joint strings 


## Execution Time

* Init

## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

/*
    Sout strjoin Ssep, Sstrings[]
    
    Join multiple strings into one

    
*/

instr 1
    Sparts[] strsplit "This;is;a;string!", ";"
    Sjoint strjoin "--", Sparts
    prints "Result: '%s'\n", Sjoint
    turnoff
endin

instr 2
    Sjoint strjoin ", ", "This", "is", "a", "string!"
    prints "Result 2: '%s'\n", Sjoint
    turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1
i2 0 1
; f0 3600

</CsScore>
</CsoundSynthesizer>



```

## See also

* [strsplit](strsplit.md)
* [strstrip](http://www.csound.com/docs/manual/strstrip.html)
* [strsub](http://www.csound.com/docs/manual/strsub.html)
* [strindex](http://www.csound.com/docs/manual/strindex.html)
* [pathSplit](pathSplit.md)
* [pathSplitExt](pathSplitExt.md)


## Credits

Eduardo Moguillansky, 2021