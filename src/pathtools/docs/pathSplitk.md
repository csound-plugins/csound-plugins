# pathSplitk

## Abstract

Split a path into directory and basename at perf-time

## Description

Given a path `/path/to/filename.txt`, split it in `/path/to` and `filename.txt`. The directory part will never end with a path separator unless it is the root path

## Syntax

    Sdirectory, Sbase pathSplitk Spath

## Arguments

* `Spath`: The path to split

## Output

* `Sdirectory`: the directory part of the path
* `Sbase`: the basename part of the path

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
    Sdir, Sbase pathSplitk Spath
    printsk "Spath: \"%s\", Sdir: \"%s\", Sbase: \"%s\"\n", Spath, Sdir, Sbase

    Spath = "filename.ext"
    Sdir, Sbase pathSplitk Spath
    printsk "Spath: \"%s\", Sdir: \"%s\", Sbase: \"%s\"\n", Spath, Sdir, Sbase
        
    Spath = "/filename.ext"
    Sdir, Sbase pathSplitk Spath
    printsk "Spath: \"%s\", Sdir: \"%s\", Sbase: \"%s\"\n", Spath, Sdir, Sbase
    
    turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1

```

</CsScore>
</CsoundSynthesizer>


## See also

* [pathSplit](pathSplit.md)
* [pathSplitExt](pathSplitExt.md)
* [pathSplitExtk](pathSplitExtk.md)


## Credits

Eduardo Moguillansky, 2020
