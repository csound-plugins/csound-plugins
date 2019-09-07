<CsoundSynthesizer>
<CsOptions>
-odac           

</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
    kgaindb = linlin:k(ramptrig(metro(0.25), 4), -40, +6)
    kfreq = 1000
    asig vco2 ampdb(kgaindb), kfreq
    // alim compress2 asig, asig, -200, -5, -1, 100, 0.002, 0.1, 0.002
    alim peaklim asig, -6
    kred = rms(asig) / max(0.0000000000001, rms(alim))
    outs alim, alim
    printf "max red: %.1f \r", timeinstk(), dbamp:k(kred)
endin

</CsInstruments>

<CsScore>

i1 0 12

</CsScore>
</CsoundSynthesizer>