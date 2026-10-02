<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

giCount = 16
giXs[] fillarray 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
giYs[] fillarray 0, .5, 1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 6, 6.5, 7, 7.5
giTable ftgen 1, 0, -32, -2, 0, 0, 1, .5, 2, 1, 3, 1.5, \
  4, 2, 5, 2.5, 6, 3, 7, 3.5, 8, 4, 9, 4.5, 10, 5, 11, 5.5, \
  12, 6, 13, 6.5, 14, 7, 15, 7.5

; Run one instrument family at a time to compare total CPU time with -m128.
instr KArraySearch
  kX line 0, p3, giCount - 1
  kIndex bisect kX, giXs
  kValue interp1d kIndex, giYs
  kSink = kValue
endin

instr AudioArraySearch
  aX line 0, p3, giCount - 1
  aIndex bisect aX, giXs
  aValue interp1d aIndex, giYs
  out aValue * 0
endin

instr KTableSearch
  kX line 0, p3, giCount - 1
  kIndex bisect kX, giTable, 2, 0
  kValue interp1d kIndex, giTable, "linear", 2, 1
  kSink = kValue
endin

instr AudioTableSearch
  aX line 0, p3, giCount - 1
  aIndex bisect aX, giTable, 2, 0
  aValue interp1d aIndex, giTable, "linear", 2, 1
  out aValue * 0
endin
</CsInstruments>
<CsScore>
; Uncomment one workload at a time; use the same duration and voice count when
; comparing revisions. Increase the voice count to amplify opcode cost.
i "KArraySearch" 0 20 32
; i "AudioArraySearch" 0 20 32
; i "KTableSearch" 0 20 32
; i "AudioTableSearch" 0 20 32
e
</CsScore>
</CsoundSynthesizer>
