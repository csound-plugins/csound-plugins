# pathAbsolute

## Abstract

Returns the absolute path of a file


## Description

If a relative path is given as argument, pathAbsolute returns the absolute path
relative to the current working directory. Bear in mind that the current working 
directory is not necessarily the directory of the csd being run but the directory 
in which csound was started (either directly or via the API). 

!!! Note

    To calculate the absolute path in relation to the directory of the script
    being run, see `scriptDir`

## Syntax

    Sabspath pathAbsolute Spath 
        
## Arguments

* `Spath': The path to a file or directory

## Output

* `Sabspath`: the absolute path

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
    pathAbsolute

    Returns the absolute path of a file. 
*/


sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
    Spath = "/home/bar"
    Sabs = pathAbsolute(Spath)
    prints "Path: \"%s\", Absolute Path: \"%s\" \n", Spath, Sabs

    Spath = "home/foo.ext"
    Sabs = pathAbsolute(Spath)
    prints "Path: \"%s\", Absolute Path: \"%s\" \n", Spath, Sabs
    
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

* [scriptDir](scriptDir.md)
* [pathJoin](pathJoin.md)
* [pathSplit](pathSplit.md)

## Credits

Eduardo Moguillansky, 2020
