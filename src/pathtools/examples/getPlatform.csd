<CsoundSynthesizer>
<CsOptions>
--nosound

</CsOptions>

<CsInstruments>
/*
    Sdir scriptDir

    get the directory of the loaded script. This is not necessarily
    the same as the current working directory. In parituclar, if
    csound is launched as 

    $ /home/foo/> csound baz/bar/myscript.csd

    The current working dir is /home/foo, whereas the script directory
    is /home/foo/baz/bar

*/

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
    prints "\n\n"
    
    Sscriptdir scriptDir
    prints "Directory of current script: %s \n\n", Sscriptdir

    Scwd pwd
    prints "Current working dir: %s\n", Scwd

    turnoff

endin

</CsInstruments>

<CsScore>

i1 0 1
; f0 3600

</CsScore>
</CsoundSynthesizer>
