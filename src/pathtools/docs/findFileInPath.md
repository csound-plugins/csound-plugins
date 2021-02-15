# findFileInPath

## Abstract

Find a file inside the search paths of the csound environment


## Description

Search for an existing file as csound would search when ,for example,
loading a sample. Returns the absolute path

## Syntax

    Spath fileFind Sfile 
        
## Arguments

* `Sfile`: a relative path to an existing file

## Output

* `Sout`: the absolute path, or "" if not found

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
    Sabspath fileFind Sfile

    Searched for Sfile, first in the current directory, then in the
    directories specified in SSDIR. Returns the absolute path or 
    an empty string to signal that the file was not found.

*/

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
    ; find the directory of this script
    Spath findFile "fileFind.csd"
    Sdir, Sbase pathSplit Spath
    prints "Folder: %s \n", Sdir

    Svalue getEnvVar "SSDIR"
    prints "SSDIR: %s \n", Svalue
 
    Sscriptdir scriptDir
    prints "Directory of current script: %s \n", Sscriptdir
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

## Credits

Eduardo Moguillansky, 2020
