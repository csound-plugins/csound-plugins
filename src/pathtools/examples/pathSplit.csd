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
    Sdir, Sbase pathSplit Spath
    prints "Spath: \"%s\", Sdir: \"%s\", Sbase: \"%s\"\n", Spath, Sdir, Sbase

    Spath = "filename.ext"
    Sdir, Sbase pathSplit Spath
    prints "Spath: \"%s\", Sdir: \"%s\", Sbase: \"%s\"\n", Spath, Sdir, Sbase
        
    Spath = "/filename.ext"
    Sdir, Sbase pathSplit Spath
    prints "Spath: \"%s\", Sdir: \"%s\", Sbase: \"%s\"\n", Spath, Sdir, Sbase
    
    turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1
; f0 3600

</CsScore>
</CsoundSynthesizer>
