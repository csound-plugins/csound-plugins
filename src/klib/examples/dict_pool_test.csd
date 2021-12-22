<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 2
    idict1 dict_new "sf", "foo", 1, "bar", 10
    prints "idict1: %d \n", idict1

    idict2 dict_new "sf", "foo", 1, "bar", 10
    prints "idict2: %d \n", idict2
    turnoff

endin

instr 1
    idict1 dict_new "sa", "foo", 1, "bar", 10, "baz", "bazvalue"
    prints "idict1: %d \n", idict1

    idict2 dict_new "sa", "foo", 1, "bar", 10, "baz", "bazvalue"
    prints "idict2: %d \n", idict2
    turnoff
endin

instr 3
    prints "\n --------------------------------------- \n"
    i0 = 0
    while i0 < 400 do
        idict dict_new "sa", "i0", i0, "dictname", sprintf("dict%d", i0), "idx", 0
        dict_set idict, "idx", idict
        dict_print idict
        i0 += 1
    od
    turnoff
endin

</CsInstruments>

<CsScore>

i3 0 0.1
i3 + 0.1

; f0 3600

</CsScore>
</CsoundSynthesizer>
