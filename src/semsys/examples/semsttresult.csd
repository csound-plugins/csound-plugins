<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semsttresult.csd
;
; semsttresult pops the oldest finished transcription (FIFO) from the STT worker,
; plus its length in characters. Gate it with semsttready. Run in real time
; (-o dac) so the poll waits in wall-clock while the background worker transcribes.
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
    r:k = semsttready(h)
    if (r == 1) then
        text:S, len:k = semsttresult(h)     ; pops the oldest result
        if (len > 0) then                   ; skip empty (no-speech) windows
            println("TRANSCRIPTION (%d chars): %s\n", len, text)
        endif
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i "SUBMIT" 0 0.1
i "POLL"   0 120
</CsScore>
</CsoundSynthesizer>
