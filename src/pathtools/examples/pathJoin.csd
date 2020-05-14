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
