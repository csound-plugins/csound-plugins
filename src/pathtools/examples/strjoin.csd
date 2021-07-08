<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

/*
    Sout strjoin Ssep, Sstrings[]
    
    Join multiple strings into one

    
*/

instr 1
    Sparts[] strsplit "This;is;a;string!", ";"
    Sjoint strjoin "--", Sparts
    prints "Result: '%s'\n", Sjoint
    turnoff
endin

instr 2
    Sjoint strjoin ", ", "This", "is", "a", "string!"
    prints "Result 2: '%s'\n", Sjoint
    turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1
i2 0 1
; f0 3600

</CsScore>
</CsoundSynthesizer>
