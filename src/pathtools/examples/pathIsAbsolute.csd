<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

/*
    pathIsAbsolute
    
    i_isabsolute pathIsAbsolute Spath
    k_isabsolute pathIsAbsolute Spath

    Returns 1 if Spath is an absolute path, 0 if it isn't

*/


sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
    prints "\n\n"
    
    Splatform sysPlatform

    if strcmp(Splatform, "windows") == 0 then
        Spath = "C:/home/bar"
        prints "Path %s is absolute? %d\n", Spath, pathIsAbsolute:i(Spath)

    else
        Spath = "/home/bar"
        prints "Path %s is absolute? %d\n", Spath, pathIsAbsolute:i(Spath)
    endif

    Spath = "./foo"
    prints "Path %s is absolute? %d\n", Spath, pathIsAbsolute:i(Spath)

    prints "\n\n"
    turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1
; f0 3600

</CsScore>
</CsoundSynthesizer>
