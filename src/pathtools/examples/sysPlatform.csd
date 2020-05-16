<CsoundSynthesizer>
<CsOptions>
--nosound

</CsOptions>

<CsInstruments>
/*
    Sdir sysPlatform

    get a string describing the system platform

    Possible values:
        * windows
        * macos
        * linux
        * unix
        * android

*/

instr 1
    Splatform sysPlatform
    prints "System platform: %s \n", Splatform
    turnoff

endin

</CsInstruments>

<CsScore>

i1 0 1
; f0 3600

</CsScore>
</CsoundSynthesizer>
