# strsplit

## Abstract

Split a string at a given separator


## Description

Split a string into parts at a given separator. The separator is not included in any
of the parts

## Syntax

    Sparts[] strsplit Sstring, Sseparator

## Arguments

* `Sstring`: The string to split
* `Sseparator`: the delimiter used to split the string

## Output

* `Sparts`: an array of strings, holding the parts. 


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
    Sparts[] strsplit Sstring, Sseparator

    Split a string into parts at the given separator

    
*/

instr 1
    Sparts[] strsplit "This;is;a;string!", ";"
    printarray Sparts
    Slines[] strsplit {{
Line 0
Line 1
Line 2
Line 3

Line 5}}, "\n"
    printarray Slines
    turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1
; f0 3600

</CsScore>
</CsoundSynthesizer>



```

## See also

* [strstrip](http://www.csound.com/docs/manual/strstrip.html)
* [strsub](http://www.csound.com/docs/manual/strsub.html)
* [strindex](http://www.csound.com/docs/manual/strindex.html)
* [pathSplit](pathSplit.md)[O]
* [pathSplitExt](pathSplitExt.md)


## Credits

Eduardo Moguillansky, 2021