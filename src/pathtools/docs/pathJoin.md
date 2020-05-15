# pathJoin

## Abstract

Join two parts of a path according to the current platform


## Description

Given a directory and a filename, or a base directory and a relative path, join 
these according to the current platform


## Syntax

    Sout pathJoin Spath1, Spath2 
        
## Arguments

* `Spath1`: The first part to join
* `Spath2`: The second part to join


## Output

* `Sout`: the result of joining both paths

## Execution Time

* Init 

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
    Sdir = "/home/bar"
    Sbase = "filename.ext"
    Spath = pathJoin(Sdir, Sbase)
    prints "Spath: \"%s\", Sdir: \"%s\", Sbase: \"%s\"\n", Spath, Sdir, Sbase

    Sdir = "/home/bar/"
    Sbase = "filename.ext"
    Spath = pathJoin(Sdir, Sbase)
    prints "Spath: \"%s\", Sdir: \"%s\", Sbase: \"%s\"\n", Spath, Sdir, Sbase

    Sdir = ""
    Sbase = "filename.ext"
    Spath = pathJoin(Sdir, Sbase)
    prints "Spath: \"%s\", Sdir: \"%s\", Sbase: \"%s\"\n", Spath, Sdir, Sbase

    Sdir = "/home/bar"
    Sbase = ""
    Spath = pathJoin(Sdir, Sbase)
    prints "Spath: \"%s\", Sdir: \"%s\", Sbase: \"%s\"\n", Spath, Sdir, Sbase

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

* [pathSplit](pathSplit.md)
* [pathSplitExt](pathSplitExt.md)

## Credits

Eduardo Moguillansky, 2020
