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

; audio embedder: -1 = run full, no sequence cap (PANNs CNN14 pools over time)
h_aemb@global:i = semload(-1, $AUDIO_EMB_DIR)

; --- LIVE: stream a-rate audio -> embeddings on a worker thread -----------------
; iwindow seconds per embedding (default 10, min 1). kgate == 1 the pass a fresh
; window has been embedded; kemb holds the last vector until the next one.
; PANNs embeddings are often sparse/non-negative, so any single coordinate can be zero.
instr LIVE
    sig:a = diskin2($AUDIO_TEST_FILE, 1)                ; or inch(1) for a live mic
    kemb:k[], kgate:k = semembedaudio(h_aemb, sig, 2)   ; 2 s windows
    if (kgate == 1) then
        check_coeffs:k = sqrt(sum(kemb^2))
        println("fresh audio embedding ready (dim %d) | coeffs check: %.3f", lenarray(kemb), check_coeffs)
    endif
endin

</CsInstruments>
<CsScore>

i "LIVE"   4 20

</CsScore>
</CsoundSynthesizer>
