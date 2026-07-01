<CsoundSynthesizer>
<CsOptions>
-o dac2
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; sem_stt_live.csd
;
; Voice-controlled latent space. Speak into the mic; usable speech windows are
; transcribed and the text is used as a QUERY into a semantic space, so your
; speech navigates the space and drives sound:
;
;   mic -> semsttsubmitlive (built-in speech gate) -> worker transcribes
;       -> semsttresult (text) -> semspacequery (nearest vector + score)
;       -> the match score drives a synth voice
;
; Needs both an end-to-end STT model and a sentence-embedding model, plus a
; corpus.txt to build the space from. Transcription runs on a worker thread, so
; the audio never stalls.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

#define STT_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/whisper-core/model_e2e" #
#define EMB_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/all-MiniLM-L6-v2/all-MiniLM-L6-v2-e2e" #

; speech-to-text + embedding model
h_stt@global:i = semsttload($STT_DIR, 448, 64)
h_emb@global:i = semload(256, $EMB_DIR)

; build a small semantic space from a corpus, then open it
semspacebuild(h_emb, "corpus.espc", "corpus.txt")
h_space@global:i = semspace(h_emb, "corpus.espc")

; NOTE: use a SPEECH source (clear talking). Whisper transcribes speech reliably; on
; music/singing it often returns empty, so windows come back with len == 0.
instr LISTEN
    sig:a = diskin2("/Users/pm/AcaHub/AudioSamples/spoken.wav", 1) ; or inch(1) for live mic
    semsttsubmitlive(h_stt, sig, 1) ; preferred window length; backend closes on usable speech boundary
endin

instr POLL
    r:k = semsttready(h_stt)
    if (r == 1) then
        text:S, tlen:k = semsttresult(h_stt)
        println("wait for speech...")
        println("[STT len=%d]:<%s>", tlen, text)
        if (tlen > 0) then ; skip empty (no-speech) windows
            ; query the space with the spoken text -> nearest vector + score
            neighs:k[][], score:k[] = semspacequery(h_space, text, 3)

            println("match score: %f", score[0])

            ; the semantic match score drives a sound
            event("i", "VOICE", 0, 3, score[0])
        endif
    endif
endin

; a short tone whose pitch tracks the semantic match score
instr VOICE
    freq:i = 110 + p4 * 660
    env:a = expseg(0.001, 0.001, 1, p3 - 0.001, 0.0001)
    a1:a = poscil(0.5, freq)
    out(a1 * env, a1 * env)
endin

</CsInstruments>
<CsScore>
i "LISTEN" 0 100
i "POLL" 0 100
</CsScore>
</CsoundSynthesizer>
