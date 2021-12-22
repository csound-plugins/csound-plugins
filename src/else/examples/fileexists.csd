<CsoundSynthesizer>
<CsOptions>
   
</CsOptions>

<CsInstruments>

/*
    Example file for file_exists

    file_exists returns 1 if a given path refers to an existing file
   
*/

ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
	iexists file_exists "file_exists.csd"
    print iexists
    turnoff
endin

</CsInstruments>

<CsScore>
i1 0 1

</CsScore>
</CsoundSynthesizer>
