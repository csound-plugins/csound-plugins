<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semsttsubmitft.csd
;
; semsttsubmitft transcribes the samples held in a function table (engine sr,
; mono). The whole table (flen samples) is used and segmented automatically if it
; is longer than the model window. Here: load a WAV into a mono table and submit it.
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
    semsttsubmitft(h, faudio)
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
