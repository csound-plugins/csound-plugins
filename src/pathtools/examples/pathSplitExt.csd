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
