<CsoundSynthesizer>
<CsOptions>

</CsOptions>

<CsInstruments>
sr     = 48000
ksmps  = 64
nchnls = 2
0dbfs  = 1


instr 1
  ipitch = p4
  iamp = db(p5)
  ipan = p6
  prints "note %f\n", ipitch
  ifreq = mtof:i(ipitch)
  asig = vco2:a(iamp, ifreq)
  asig = lpf18(asig, ifreq * 8, 0.85, 0) * 4
  asig *= adsr(0.005, 0.05, 0.2, p3*0.5) 
  aL, aR pan2 asig, ipan
  chnmix aL, "rev1"
  chnmix aR, "rev2"
endin

instr 2
  ipitches[] genarray 48, 72, 0.25
  i0 = 0
  iN = lenarray(ipitches)
  while i0 < iN do
    ipan = (i0 / iN * 4) % 1
	schedule 1, i0*0.4, 0.2, ipitches[i0], -3, ipan
	i0 += 1
  od
endin

instr 10
  aL chnget "rev1"
  aR chnget "rev2"
  arev1, arev2 zitarev aL, aR, "drywet", -0.25, "level", -3, "delayms", 30, "decaylow", 4, "decaymid", 2.5, "decaylfx", 300, "eq1level", 4, "eq2level", 3
  outch 1, arev1, 2, arev2
  chnclear "rev1", "rev2"
endin

</CsInstruments>

<CsScore>
i 2  0 0
i 10 0 60

</CsScore>
</CsoundSynthesizer>
