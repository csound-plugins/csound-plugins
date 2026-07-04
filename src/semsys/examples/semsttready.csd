<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semsttready.csd
;
; semsttready is a cheap non-blocking k-rate poll: it returns 1 when a finished
; transcription is waiting in the result queue. Use it to gate semsttresult.
; Run in real time (-o dac) so the poll waits in wall-clock for the worker.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 128
nchnls = 1
0dbfs = 1

#define STT_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/whisper-core/model_e2e" #
#define AUDIO   # "spoken.wav" #

h@global:i = semsttload($STT_DIR, 448, 64)

instr SUBMIT
    semsttsubmitfile(h, $AUDIO)
endin

instr POLL
    r:k = semsttready(h)                    ; 1 when a result is ready
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
