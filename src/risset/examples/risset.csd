<CsoundSynthesizer>
<CsOptions>
--nosound

</CsOptions>

<CsInstruments>
/*
    Sout risset Scommand

    Query the risset package manager

    Possible commands:
    * root -- risset's root folder
    * assets -- where risset places extra assets

*/

instr 1
    Sroot risset "root"
	prints "risset root: %s\n", Sroot
    
    Sassets risset "assets"
    prints "risset assets: %s\n", Sassets
    turnoff
endin

</CsInstruments>

<CsScore>

i1 0 0.1
; f0 3600

</CsScore>
</CsoundSynthesizer>
