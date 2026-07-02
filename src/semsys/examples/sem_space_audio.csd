<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; sem_space_audio.csd
;
; An AUDIO semantic space: index sounds by their audio embedding, then retrieve
; the nearest sounds to a query sound (audio -> audio similarity, no text).
;
;   semspacebuild(audioh, ...)   build a .espc from a folder of .wav (universal
;                                build: the audio model is picked from the handle)
;   semspace(audioh, ...)        load it (the handle only anchors ldim)
;   semspaceaddaudio(...)        add more sounds in RAM, per-op model
;   semspacequeryaudio(...)      nearest sounds to a query .wav (mean-pooled)
;   semspacequeryaudioft(...)    REAL-TIME query from an ftable (live audio, no file):
;                                capture into a circular table, query on a trigger, with
;                                a minimum-duration gate
;
; The space is a pure store: the embedding model is passed to each add/query, so
; text and audio vectors of matching dim could even share one space.
;
; Audio files must be PCM16 WAV. Model = an end-to-end audio embedder (e.g. PANNs
; CNN14); -1 = no sequence cap.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define AUDIO_MODEL # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/PANNs_CNN14_ONNX" #
#define AUDIO_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/audio_space_wav" #
#define TLEN # sr * 3 #

rec@global:i = ftgen(0, 0, $TLEN, 2, 0)

a_handle@global:i = semload(-1, $AUDIO_MODEL)
; build an .espc from every .wav in a directory (audio model -> audio embedding)
semspacebuild(a_handle, "sounds.espc", $AUDIO_DIR)

; load it back into RAM (handle only fixes the dimension)
s_handle@global:i = semspace(a_handle, "sounds.espc")

instr QUERY
    ; 3 stored sounds most similar to a query sound (file split into ~10s windows,
    ; embedded, mean-pooled into one query vector, then top-k cosine search)
    neighs:i[][], scores:i[] = semspacequeryaudio(s_handle, a_handle, "/Users/pm/AcaHub/AudioSamples/piano_samples/mallets_on_piano.wav", 3)
    printarray(scores)
    turnoff
endin

; --- REAL-TIME query from live audio (no file) ---------------------------------
; RECORD continuously overwrites the circular table `girec` with the mic input.
instr RECORD
    a1:a = inch(1)
    andx:a = phasor(sr / ftlen(rec))     ; 0..1 ramp, wraps once per table length
    tablew(a1, andx, rec, 1)             ; ixmode 1 -> normalized index
endin

; LIVE_QUERY queries the space from the capture table every 4 s. iminsec = 2 skips the
; query until at least 2 s of audio has been captured. The k-rate form snapshots the table
; on trigger and runs inference/search on a worker thread.
instr LIVE_QUERY
    ktrig:k = metro(0.25) ; ~ every 4 s
    neighs:k[][], scores:k[], kgate:k = semspacequeryaudioft(s_handle, a_handle, rec, 3, ktrig, 2)
    if (kgate == 1) then
        println("nearest match score = %f", scores[0])
    endif
endin

</CsInstruments>
<CsScore>
i "ADD_MORE" 0 0.1
i "QUERY"    1 0.1
; i "RECORD"      2 30
; i "LIVE_QUERY"  2 30
</CsScore>
</CsoundSynthesizer>
