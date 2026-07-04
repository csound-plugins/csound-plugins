<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; sem_embed_audio.csd
;
; Audio embedding with an end-to-end ONNX model (e.g. PANNs CNN14). Raw audio ->
; a semantic vector, NO classification. Two paths:
;
;   FILE (i-time) : semembedaudiofile / semembedaudioft -> 2D [nchunks, ldim],
;                   one L2-normalized row per ~10 s window. Runs in the init pass
;                   (off the audio thread), so blocking there is harmless.
;
;   LIVE (a-rate) : semembedaudio accumulates asig into windows and runs the model
;                   on a BACKGROUND worker (like STT), so the audio never stalls.
;                   kgate == 1 on the pass a fresh embedding arrives; near-silent
;                   windows are skipped.
;
; The model works at 32 kHz internally and has global time pooling, so it accepts
; any length and imposes no sequence cap -> load with maxlen = -1 ("full"). semsys
; resamples/downmixes to the model's rate in C before inference.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define AUDIO_EMB_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/PANNs_CNN14_ONNX" #
#define AUDIO_TEST_FILE # "mallets_on_piano.wav" #

audio_table@global:i = ftgen(0, 0, 0, 1, $AUDIO_TEST_FILE, 0, 0, 0)

; audio embedder: -1 = run full, no sequence cap (PANNs CNN14 pools over time)
h_aemb@global:i = semload(-1, $AUDIO_EMB_DIR)


; --- FTABLE: same, from a function table of samples (at engine sr) --------------
instr FTABLE
    emb:i[][] = semembedaudioft(h_aemb, audio_table)
    printarray(emb)
    turnoff
endin

</CsInstruments>
<CsScore>

i "FTABLE" 2 1

</CsScore>
</CsoundSynthesizer>
