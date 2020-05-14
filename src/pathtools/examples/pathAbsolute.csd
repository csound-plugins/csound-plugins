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
