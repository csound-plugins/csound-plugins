
<CsoundSynthesizer>

<CsOptions>
-odac
-d
</CsOptions>

<CsInstruments>

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

    
FLpanel "NP1136 (Peak Limiter)", 443, 788, 50, 50
FLcolor 150, 100, 150, 200, 100, 250
i__w, i__h, i__line = 300, 26, 52
iy, i__marginx = 30, 30
i_v1 FLvalue "", 50, 26, 333, iy
gkThreshold, i_s1 FLslider "Threshold (dB)", -40.0, 0.0, 0, 3, i_v1, i__w, i__h, i__marginx, iy
iy += i__line
i_v2 FLvalue "", 50, 26, 333, iy
gkRatio, i_s2 FLslider "Ratio (20:1 - PD Mode)", 1.0, 20.0, 0, 3, i_v2, i__w, i__h, i__marginx, iy
iy += i__line
i_v3 FLvalue "", 50, 26, 333, iy
gkAttack, i_s3 FLslider "Attack (µs)", 0.0, 100.0, 0, 3, i_v3, i__w, i__h, i__marginx, iy
iy += i__line
i_v4 FLvalue "", 50, 26, 333, iy
gkRelease, i_s4 FLslider "Release (ms)", 0.0, 100.0, 0, 3, i_v4, i__w, i__h, i__marginx, iy
iy += i__line
i_v5 FLvalue "", 50, 26, 333, iy
gkDetector, i_s5 FLslider "Detector HP (Hz)", 0.0, 100.0, 0, 3, i_v5, i__w, i__h, i__marginx, iy
iy += i__line
i_v6 FLvalue "", 50, 26, 333, iy
gkGr, i_s6 FLslider "GR Limit (Off / Y dB)", -40.0, 0.0, 0, 3, i_v6, i__w, i__h, i__marginx, iy
iy += i__line
i_v7 FLvalue "", 50, 26, 333, iy
gkMakeup, i_s7 FLslider "Makeup Gain (dB)", 0.0, 30.0, 0, 3, i_v7, i__w, i__h, i__marginx, iy
iy += i__line
i_v8 FLvalue "", 50, 26, 333, iy
gkTilt, i_s8 FLslider "Tilt EQ Center (Hz)", 0.0, 100.0, 0, 3, i_v8, i__w, i__h, i__marginx, iy
iy += i__line
i_v9 FLvalue "", 50, 26, 333, iy
gkTilt, i_s9 FLslider "Tilt EQ Low/High (dB)", -6.0, 6.0, 0, 3, i_v9, i__w, i__h, i__marginx, iy
iy += i__line
i_v10 FLvalue "", 50, 26, 333, iy
gkWet, i_s10 FLslider "Wet Mix (%)", 0.0, 100.0, 0, 3, i_v10, i__w, i__h, i__marginx, iy
iy += i__line
i_v11 FLvalue "", 50, 26, 333, iy
gkProcessing, i_s11 FLslider "Processing Mode", 0.0, 1.0, 0, 3, i_v11, i__w, i__h, i__marginx, iy
iy += i__line
i_v12 FLvalue "", 50, 26, 333, iy
gkDetector, i_s12 FLslider "Detector Mode", 0.0, 1.0, 0, 3, i_v12, i__w, i__h, i__marginx, iy
iy += i__line
i_v13 FLvalue "", 50, 26, 333, iy
gkDetector, i_s13 FLslider "Detector Input", 0.0, 1.0, 0, 3, i_v13, i__w, i__h, i__marginx, iy
iy += i__line
i_v14 FLvalue "", 50, 26, 333, iy
gkHard, i_s14 FLslider "Hard Clip", 0.0, 1.0, 0, 3, i_v14, i__w, i__h, i__marginx, iy
FLpanelEnd
FLrun
FLsetVal_i -12.0, i_s1		 ; Threshold (dB)
FLsetVal_i 4.0, i_s2		 ; Ratio (20:1 - PD Mode)
FLsetVal_i 30.0, i_s3		 ; Attack (µs)
FLsetVal_i 45.0, i_s4		 ; Release (ms)
FLsetVal_i 0.0, i_s5		 ; Detector HP (Hz)
FLsetVal_i -18.0, i_s6		 ; GR Limit (Off / Y dB)
FLsetVal_i 0.0, i_s7		 ; Makeup Gain (dB)
FLsetVal_i 50.0, i_s8		 ; Tilt EQ Center (Hz)
FLsetVal_i 0.0, i_s9		 ; Tilt EQ Low/High (dB)
FLsetVal_i 100.0, i_s10		 ; Wet Mix (%)
FLsetVal_i 0.0, i_s11		 ; Processing Mode
FLsetVal_i 1.0, i_s12		 ; Detector Mode
FLsetVal_i 0.0, i_s13		 ; Detector Input
FLsetVal_i 0.0, i_s14		 ; Hard Clip


instr 1
  ; xxx
endin

</CsInstruments>

<CsScore>

i1 0 3600

</CsScore>
</CsoundSynthesizer>
