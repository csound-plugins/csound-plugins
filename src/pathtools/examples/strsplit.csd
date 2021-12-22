<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

/*
    Sparts[] strsplit Sstring, Sseparator

    Split a string into parts at the given separator

    
*/

instr 1
    Sparts[] strsplit "This;is;a;string!", ";"
    printarray Sparts
    Slines[] strsplit {{
Line 0
Line 1
Line 2
Line 3

Line 5}}, "\n"
    printarray Slines
    turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1
; f0 3600

</CsScore>
</CsoundSynthesizer>
