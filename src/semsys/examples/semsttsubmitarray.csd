<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semsttsubmitarray.csd
;
; semsttsubmitarray transcribes a 1-D array of mono samples (engine sr, -1..1).
; The array length is whatever you filled, NOT limited by ksmps. Here: load a WAV
; into a table, copy it into an i-array, and submit that array to the worker.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 128
nchnls = 1
0dbfs = 1

#define STT_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/whisper-core/model_e2e" #
#define AUDIO   # "spoken.wav" #

faudio@global:i = ftgen(0, 0, 0, 1, $AUDIO, 0, 0, 1)   ; mono table (GEN01, channel 1)
h@global:i = semsttload($STT_DIR, 448, 64)

instr SUBMIT
    samps:i[] = init(ftlen(faudio))
    copyf2array(samps, faudio)
    semsttsubmitarray(h, samps)
endin

instr POLL
    r:k = semsttready(h)
    if (r == 1) then
        text:S, len:k = semsttresult(h)
        println("TRANSCRIPTION: %s\n", text)
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i "SUBMIT" 0 0.1
i "POLL"   0 120
</CsScore>
</CsoundSynthesizer>
