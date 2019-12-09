<CsoundSynthesizer>
<CsOptions>

--nosound
-m0

</CsOptions>

<CsInstruments>

/*
    Example file for uniqinstance

    instrnum  uniqinstance intinstr

    Returns a unique fractional instrument number which is not
    active at the moment and can be assigned to a new instance
    
*/

instr 1
	kcounter init 0
	ktrig metro 20
	if ktrig == 1 then
		kcounter += 1
		kinstr = 10 + kcounter/1000
		printk2 kinstr
		event "i", kinstr, 0, 1
	endif
endin

instr 2
	instrnum10 uniqinstance 10
	printf "Unique instance of instr 10: %f\n", 1, instrnum10
	instrnum11 uniqinstance 11
	printf "Unique instance of instr 11: %f\n", 1, instrnum11
	turnoff
endin

instr 10
    print p1
endin

instr 11
    print p1
endin

</CsInstruments>

<CsScore>
i 10.150 0 0.1
i 11 0 2
i 1 0 0.5
i 2 0.5 0.1

</CsScore>
</CsoundSynthesizer>
