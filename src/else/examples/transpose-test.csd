<CsoundSynthesizer>
<CsOptions>

</CsOptions>

<CsInstruments>
sr     = 48000
ksmps  = 64
nchnls = 2
0dbfs  = 1

giwindowtab = ftgen(0, 0, 4096, 20, 1, 1)
  


opcode nativetranspose, a, akkp
  ; native implementation by J. Heintz
  asig, ksemitones, kwindur, iwindow xin
  imaxdelay = 2
  kfreqratio = semitone(ksemitones)
  println "kfreqratio: %f", kfreqratio
  iwindowtab = ftgenonce(0, 0, 4096, 20, iwindow, 1)
  iwindowtab = giwindowtab
  kphasorfreq = (1 - kfreqratio) / kwindur
  aphasor1 = phasor:a(kphasorfreq)
  aphasor2 = phasor:a(kphasorfreq, 0.5)
  adelay1 = vdelayx(asig, aphasor1 * kwindur, imaxdelay, 4)
  adelay2 = vdelayx(asig, aphasor2 * kwindur, imaxdelay, 4)
  adelay1 *= tablei:a(aphasor1, iwindowtab, 1)
  adelay2 *= tablei:a(aphasor2, iwindowtab, 1)
  adelay1 += adelay2
  xout adelay1
endop

instr 1
  kwindur = 0.02
  asig = vco2(0.5, mtof:i(ntom("2G")))
  asig = gtadsr(asig, 0.004, 0.008, 0.2, 0.3, metro(7/3))
  ktime = eventtime()
  kshift = bpf:k(ktime, 0, 0, 8, 12, 12, 12, 20, 0)
  println "kshift: %f", kshift
  ashifted = nativetranspose(asig, kshift, kwindur, 1)
  outs ashifted, ashifted
endin

</CsInstruments>

<CsScore>
i1 0 20

</CsScore>
</CsoundSynthesizer>
