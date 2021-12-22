<CsoundSynthesizer>
<CsOptions>
-odac           
-d     ;;;RT audio I/O

</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

FLpanel "Chebyshev Filter", 443, 600, 50, 50
FLcolor 150, 100, 150, 200, 100, 250
i__w, i__h, i__line = 300, 30, 60
iy, i__marginx = 30, 30
i_v1 FLvalue "", 50, 30, 333, iy
gkProcessing, i_s1 FLslider "Processing", 0.0, 1.0, 0, 3, i_v1, i__w, i__h, i__marginx, iy
iy += i__line
i_v2 FLvalue "", 50, 30, 333, iy
gkFilter, i_s2 FLslider "Filter Type", 0.0, 2.0, 0, 3, i_v2, i__w, i__h, i__marginx, iy
iy += i__line
i_v3 FLvalue "", 50, 30, 333, iy
gkCutoff, i_s3 FLslider "Cutoff (Scale)", 0.0, 100.0, 0, 3, i_v3, i__w, i__h, i__marginx, iy
iy += i__line
i_v4 FLvalue "", 50, 30, 333, iy
gkPassband, i_s4 FLslider "Passband ripple (Less/More)", 0.0, 0.9, 0, 3, i_v4, i__w, i__h, i__marginx, iy
iy += i__line
i_v5 FLvalue "", 50, 30, 333, iy
gkOutput, i_s5 FLslider "Output (dB)", -25.0, 25.0, 0, 3, i_v5, i__w, i__h, i__marginx, iy
iy += i__line
i_v6 FLvalue "", 50, 30, 333, iy
gkLimiter, i_s6 FLslider "Limiter", 0.0, 1.0, 0, 3, i_v6, i__w, i__h, i__marginx, iy
FLpanelEnd
FLrun
FLsetVal_i 0.0, i_s1		 ; Processing
FLsetVal_i 0.0, i_s2		 ; Filter Type
FLsetVal_i 100.0, i_s3		 ; Cutoff (Scale)
FLsetVal_i 0.3, i_s4		 ; Passband ripple (Less/More)
FLsetVal_i 0.0, i_s5		 ; Output (dB)
FLsetVal_i 0.0, i_s6		 ; Limiter

instr 1
endin

</CsInstruments>

<CsScore>

i1 0 100
; f0 3600

</CsScore>
</CsoundSynthesizer>