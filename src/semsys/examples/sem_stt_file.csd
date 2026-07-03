<CsoundSynthesizer>
<CsOptions>
-o dac2
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; sem_stt_file.csd
;
; Offline transcription of an audio file. Shows the basic asynchronous flow:
;   semsttsubmitfile  -> enqueue the file (returns immediately, no text yet)
;   semsttready       -> poll until the worker has finished
;   semsttresult      -> read the transcription
;
; Run in real time (-odac) so the poll waits in wall-clock while the background
; worker transcribes. No sound is produced; the result is printed.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 128
nchnls = 1
0dbfs = 1

; end-to-end STT model dir: must contain model.onnx; keep model.onnx.data there too if present
#define STT_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/whisper-core/model_e2e" #
#define AUDIO   # "/Users/pm/AcaHub/AudioSamples/spoken.wav" #

faudio@global:i = ftgen(0, 0, 0, 1, $AUDIO, 0, 0, 0)


h@global:i = semsttload($STT_DIR, 448, 64)

; submit the file once at init; transcription runs on the worker thread
instr SUBMIT
    semsttsubmitfile(h, $AUDIO)
    ; audio_arr:i[] = init(ftlen(faudio))
    ; copyf2array(audio_arr, faudio)
    ; semsttsubmitarray(h, audio_arr)
    ; semsttsubmitft(h, faudio)
endin

; poll until the transcription is ready, print it, stop
instr POLL
    r:k = semsttready(h)
    if (r == 1) then
        text:S, len:k = semsttresult(h)
        println("TRANSCRIPTION: %s\n", text)
    endif
endin

</CsInstruments>
<CsScore>
i "SUBMIT" 0 0.1
i "POLL"   0 120
</CsScore>
</CsoundSynthesizer>
