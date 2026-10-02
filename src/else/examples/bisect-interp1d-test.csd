<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

giPairs ftgen 1, 0, -8, -2, 0, 100, 1, 101, 2, 102, 3, 103
giXTable ftgen 2, 0, -8, -2, 0, 1, 1, 2, 2, 3, 3, 4
giIdent ftgen 3, 0, -8, -2, 0, 1, 2, 3, 4, 5, 6, 7

instr Test
  iYs[] fillarray 0, 10, 20, 30
  kInputs[] fillarray -1, 0.5, 4, 1.5
  kValues[] interp1d kInputs, iYs
  if lenarray(kValues) != 4 || \
     abs(kValues[0]) > 1e-6 || abs(kValues[1] - 5) > 1e-6 || \
     abs(kValues[2] - 30) > 1e-6 || abs(kValues[3] - 15) > 1e-6 then
    printks "interp1d k-array clamp regression\n", 0
    exitnowk -1
  endif

  iCubic[] fillarray 0, 10, 20, 100
  aIndex = 1.5
  aCubic interp1d aIndex, iCubic, "cubic"
  kCubic downsamp aCubic
  if abs(kCubic - 10.625) > 1e-6 then
    printks "interp1d audio cubic regression: got %g expected 10.625\n", 0, kCubic
    exitnowk -1
  endif

  kTableInputs[] fillarray -1, 0.5, 4, 1.5
  kIndices[] bisect kTableInputs, giXTable, 2, 0
  if lenarray(kIndices) != 4 || \
     abs(kIndices[0]) > 1e-6 || abs(kIndices[1] - 0.5) > 1e-6 || \
     abs(kIndices[2] - 3) > 1e-6 || abs(kIndices[3] - 1.5) > 1e-6 then
    printks "bisect strided table regression\n", 0
    exitnowk -1
  endif

  kValue interp1d 1.5, giPairs, "linear", 2, 1
  if abs(kValue - 101.5) > 1e-6 then
    printks "interp1d strided table regression: got %g expected 101.5\n", 0, kValue
    exitnowk -1
  endif

  aTableIndex = 1.5
  aTableValue interp1d aTableIndex, giPairs, "linear", 2, 1
  kTableValue downsamp aTableValue
  if abs(kTableValue - 101.5) > 1e-6 then
    printks "interp1d audio table regression: got %g expected 101.5\n", 0, kTableValue
    exitnowk -1
  endif

  aSimpleTable interp1d aTableIndex, giIdent
  kSimpleTable downsamp aSimpleTable
  if abs(kSimpleTable - 1.5) > 1e-6 then
    printks "interp1d audio default table regression: got %g expected 1.5\n", 0, kSimpleTable
    exitnowk -1
  endif

  kSimpleTableK interp1d 1.5, giIdent
  if abs(kSimpleTableK - 1.5) > 1e-6 then
    printks "interp1d k default table regression: got %g expected 1.5\n", 0, kSimpleTableK
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i "Test" 0 .01
e
</CsScore>
</CsoundSynthesizer>
