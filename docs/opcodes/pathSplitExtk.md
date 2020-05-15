# pathSplitExtk

## Abstract

Split a path into prefix and extension at performance time


## Description

Given a path `/path/to/filename.txt`, split it in `/path/to/filename` and `.txt`. 
See [pathSplitExt](pathSplitExt.md) for an init-time only version

## Syntax

    Sprefix, Sext pathSplitExtk Spath

## Arguments

* `Spath`: The path to split

## Output

* `Sprefix`: Everything before the extension
* `Sext`: the extension, beginning with a .

## Execution Time

* Performance

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
    Spath = "/home/bar/filename.ext"
    S1, Sext pathSplitExt Spath
    prints "Spath: \"%s\", S1: \"%s\", Sext: \"%s\"\n", Spath, S1, Sext

    Spath = "foo.filename.ext"
    S1, Sext pathSplitExt Spath
    prints "Spath: \"%s\", S1: \"%s\", Sext: \"%s\"\n", Spath, S1, Sext
        
    Spath = "/filename.ext"
    S1, Sext pathSplitExt Spath
    prints "Spath: \"%s\", S1: \"%s\", Sext: \"%s\"\n", Spath, S1, Sext
    
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

* [pathSplitExt](pathSplitExtk.md)
* [pathSplit](pathSplitk.md)
* [pathJoin](pathJoin.md)

## Credits

Eduardo Moguillansky, 2020
