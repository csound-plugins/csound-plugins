<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

/*
    semsys opcode smoke tests.

    To reproduce the full test, run this CSD from the utest/ directory or adjust
    the paths below. The fixture folders must be populated before running:

      utest_data/corpus_a.txt
      utest_data/texts/*.txt
      utest_data/audio_space_wav/*.wav
      utest_data/<some_audio_file>.wav
      utest_data/<some_audio_speech_file>.wav
      espc/

    The espc/ folder is used for generated .espc files. The test prints [PASS],
    [FAIL], [SKIP], and [INFO] messages so missing or unsuitable fixtures are
    visible in the Csound output.
*/

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define EMBED_TEXT_MODEL_DIR  # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/all-MiniLM-L6-v2/all-MiniLM-L6-v2-e2e" # // replace with your model folder
#define EMBED_AUDIO_MODEL_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/PANNs_CNN14_ONNX" # // replace with your model folder
#define STT_MODEL_DIR         # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/whisper-core/model_e2e" # // replace with your model folder

#define UTEST_DIR       # "utest_data" #
#define TEXT_FILE       # "utest_data/corpus_a.txt" #
#define TEXT_DIR        # "utest_data/texts" #
#define AUDIO_DIR       # "utest_data/audio_space_wav" #
#define AUDIO_TEST_FILE # "utest_data/mallets_on_piano.wav" #  // replace with <some_audio_file>.wav
#define STT_AUDIO_TRACK # "utest_data/spoken.wav" # // replace with <some_audio_speech_file>.wav

#define TEXT_ESPC       # "espc/semsys_utest_text.espc" #
#define TEXT_ESPC_SAVE  # "espc/semsys_utest_text_saved.espc" #
#define TEXT_ESPC_SAVEK # "espc/semsys_utest_text_saved_k.espc" #
#define AUDIO_ESPC      # "espc/semsys_utest_audio.espc" #
#define AUDIO_ESPC_SAVE # "espc/semsys_utest_audio_saved.espc" #

#define MIN_SCORE       # 0.0001 #

audio_table@global:i = ftgen(0, 0, 0, 1, $AUDIO_TEST_FILE, 0, 0, 0)
stt_table@global:i   = ftgen(0, 0, 0, 1, $STT_AUDIO_TRACK, 0, 0, 0)
rec_table@global:i   = ftgen(0, 0, sr * 3, 2, 0)

mtext@global:i  = semload(256, $EMBED_TEXT_MODEL_DIR)
maudio@global:i = semload(-1, $EMBED_AUDIO_MODEL_DIR)
mstt@global:i   = semsttload($STT_MODEL_DIR, 448, 64)

stext@global:i  = semspace(mtext)
saudio@global:i = semspace(maudio)

fail@global:k = init(0)
done@global:k = init(0)
skip@global:k = init(0)

instr LOAD_TEST
    tldim:i = semdim(mtext)
    aldim:i = semdim(maudio)

    if (tldim <= 0) then
        prints("[FAIL] semload/semdim text: invalid dim %d\n", tldim)
        fail += 1
        turnoff
    endif
    if (aldim <= 0) then
        prints("[FAIL] semload/semdim audio: invalid dim %d\n", aldim)
        fail += 1
        turnoff
    endif

    prints("[PASS] semload + semdim: text dim=%d audio dim=%d\n", tldim, aldim)
    done += 1
    turnoff
endin

instr EMBED_TEXT_I_TEST
    emb:i[][] = semembedtxt(mtext, "warm analog texture with slow evolving harmonics")
    rows:i = lenarray(emb)
    dim:i = semdim(mtext)

    isum:i = 0
    irow:i = 0
    while (irow < rows) do
        icol:i = 0
        while (icol < dim) do
            isum += emb[irow][icol] * emb[irow][icol]
            icol += 1
        od
        irow += 1
    od
    inorm:i = sqrt(isum)

    if (rows <= 0 || inorm <= 0) then
        prints("[FAIL] semembedtxt i-rate: rows=%d norm=%.6f\n", rows, inorm)
        fail += 1
        turnoff
    endif

    prints("[PASS] semembedtxt i-rate: rows=%d sqrt(sum^2)=%.6f\n", rows, inorm)
    done += 1
    turnoff
endin

instr EMBED_TEXT_FILE_TEST
    emb:i[][] = semembedtxtfile(mtext, $TEXT_FILE)
    rows:i = lenarray(emb)
    dim:i = semdim(mtext)

    isum:i = 0
    irow:i = 0
    while (irow < rows) do
        icol:i = 0
        while (icol < dim) do
            isum += emb[irow][icol] * emb[irow][icol]
            icol += 1
        od
        irow += 1
    od
    inorm:i = sqrt(isum)

    if (rows <= 0 || inorm <= 0) then
        prints("[FAIL] semembedtxtfile: rows=%d norm=%.6f\n", rows, inorm)
        fail += 1
        turnoff
    endif

    prints("[PASS] semembedtxtfile: rows=%d sqrt(sum^2)=%.6f\n", rows, inorm)
    done += 1
    turnoff
endin

instr EMBED_TEXT_K_TEST
    if (p3 < 0.5) then
        prints("[SKIP] semembedtxt.k: instrument duration too short (p3=%.3f)\n", p3)
        skip += 1
        skip += 1
        turnoff
    endif

    emb:k[], changed:k = semembedtxt(mtext, "k-rate semantic vector test")
    knorm:k = sqrt(sum(emb^2))

    if (changed == 1 && knorm > 0) then
        println("[PASS] semembedtxt.k: dim=%d sqrt(sum^2)=%.6f", lenarray(emb), knorm)
        done += 1
        turnoff
    endif

    if (timeinsts() > p3 - 0.1) then
        println("[FAIL] semembedtxt.k: no valid changed gate, norm=%.6f", knorm)
        fail += 1
        turnoff
    endif
endin

instr EMBED_AUDIO_FILE_TEST
    emb:i[][] = semembedaudiofile(maudio, $AUDIO_TEST_FILE)
    rows:i = lenarray(emb)
    dim:i = semdim(maudio)

    isum:i = 0
    irow:i = 0
    while (irow < rows) do
        icol:i = 0
        while (icol < dim) do
            isum += emb[irow][icol] * emb[irow][icol]
            icol += 1
        od
        irow += 1
    od
    inorm:i = sqrt(isum)

    if (rows <= 0 || inorm <= 0) then
        prints("[FAIL] semembedaudiofile: rows=%d norm=%.6f\n", rows, inorm)
        fail += 1
        turnoff
    endif

    prints("[PASS] semembedaudiofile: rows=%d sqrt(sum^2)=%.6f\n", rows, inorm)
    done += 1
    turnoff
endin

instr EMBED_AUDIO_FT_TEST
    if (ftlen(audio_table) <= 0) then
        prints("[SKIP] semembedaudioft: audio ftable is empty or invalid\n")
        skip += 1
        turnoff
    endif

    emb:i[][] = semembedaudioft(maudio, audio_table)
    rows:i = lenarray(emb)
    dim:i = semdim(maudio)

    isum:i = 0
    irow:i = 0
    while (irow < rows) do
        icol:i = 0
        while (icol < dim) do
            isum += emb[irow][icol] * emb[irow][icol]
            icol += 1
        od
        irow += 1
    od
    inorm:i = sqrt(isum)

    if (rows <= 0 || inorm <= 0) then
        prints("[FAIL] semembedaudioft: rows=%d norm=%.6f\n", rows, inorm)
        fail += 1
        turnoff
    endif

    prints("[PASS] semembedaudioft: rows=%d sqrt(sum^2)=%.6f\n", rows, inorm)
    done += 1
    turnoff
endin

instr EMBED_AUDIO_K_TEST
    if (p3 < 2) then
        prints("[SKIP] semembedaudio: instrument duration too short for a 1s window (p3=%.3f)\n", p3)
        skip += 1
        turnoff
    endif

    sig:a = diskin2($AUDIO_TEST_FILE, 1, 0, 1)
    emb:k[], gate:k = semembedaudio(maudio, sig, 1)
    knorm:k = sqrt(sum(emb^2))

    if (gate == 1 && knorm > 0) then
        println("[PASS] semembedaudio: dim=%d sqrt(sum^2)=%.6f", lenarray(emb), knorm)
        done += 1
        turnoff
    endif

    if (timeinsts() > p3 - 0.1) then
        println("[FAIL] semembedaudio: no valid gate, norm=%.6f", knorm)
        fail += 1
        turnoff
    endif
endin

instr SPACE_TEXT_BUILD_LOAD_QUERY_TEST
    semspacebuild(mtext, $TEXT_ESPC, $TEXT_DIR)

    s_empty:i = semspace(mtext)
    s_file:i = semspace(mtext, $TEXT_ESPC)

    paths:S[] init 1
    paths[0] = $TEXT_ESPC
    s_vs:i = semspace(mtext, paths)

    neighs:i[][], scores:i[] = semspacequerytxt(s_file, mtext, "deep resonant sound", 2)
    nscore:i = lenarray(scores)
    if (nscore == 0) then
        prints("[FAIL] semspace/semspace.f/semspace.vs/semspacebuild/semspacequerytxt: query returned no scores\n")
        fail += 1
        turnoff
    endif
    score0:i = scores[0]

    if (s_empty <= 0 || s_file <= 0 || s_vs <= 0 || score0 <= $MIN_SCORE) then
        prints("[FAIL] semspace/semspace.f/semspace.vs/semspacebuild/semspacequerytxt: handles=%d,%d,%d score=%.9f\n", s_empty, s_file, s_vs, score0)
        fail += 1
        turnoff
    endif

    semspacesave(s_file, $TEXT_ESPC_SAVE)
    prints("[PASS] semspace text build/load/query/save: score=%.9f\n", score0)
    done += 1
    turnoff
endin

instr SPACE_TEXT_ADD_I_TEST
    semspaceaddtxt(stext, mtext, "a bright metallic hit with sharp attack")
    semspaceaddtxt(stext, mtext, "a warm analog pad with slow modulation")
    prints("[INFO] semspaceaddtxt i-rate populated text space\n")
    turnoff
endin

instr SPACE_TEXT_QUERY_I_TEST
    neighs:i[][], scores:i[] = semspacequerytxt(stext, mtext, "warm analog sound", 2)
    nscore:i = lenarray(scores)
    if (nscore == 0) then
        prints("[FAIL] semspaceaddtxt + semspacequerytxt i-rate: query returned no scores; text space may be empty\n")
        fail += 1
        turnoff
    endif
    if (scores[0] <= $MIN_SCORE) then
        prints("[FAIL] semspaceaddtxt + semspacequerytxt i-rate: score=%.9f\n", scores[0])
        fail += 1
    else
        prints("[PASS] semspaceaddtxt + semspacequerytxt i-rate: score=%.9f\n", scores[0])
        done += 1
    endif
    turnoff
endin

instr SPACE_TEXT_K_ADD_TEST
    if (p3 < 0.25) then
        prints("[SKIP] semspaceaddtxt.k: instrument duration too short for trigger setup (p3=%.3f)\n", p3)
        skip += 1
        turnoff
    endif

    ktrig:k = (timeinsts() < 0.2 ? 1 : 0)
    semspaceaddtxt(stext, mtext, "grainy resonant texture", ktrig)
    semspacesave(stext, $TEXT_ESPC_SAVEK, ktrig)

    if (timeinsts() > p3 - 0.1) then
        prints("[INFO] semspaceaddtxt.k + semspacesave.k submitted\n")
        turnoff
    endif
endin

instr SPACE_TEXT_K_QUERY_TEST
    if (p3 < 1) then
        prints("[SKIP] semspacequerytxt.k: instrument duration too short for async query (p3=%.3f)\n", p3)
        skip += 1
        turnoff
    endif

    neighs:k[][], scores:k[], gate:k = semspacequerytxt(stext, mtext, "grainy texture", 1)
    if (gate == 1 && scores[0] > $MIN_SCORE) then
        println("[PASS] semspaceaddtxt.k + semspacesave.k + semspacequerytxt.k: score=%.9f", scores[0])
        done += 1
        turnoff
    endif

    if (timeinsts() > p3 - 0.1) then
        println("[FAIL] semspaceaddtxt.k + semspacesave.k + semspacequerytxt.k: no valid score, gate=%d score=%.9f", gate, scores[0])
        fail += 1
        turnoff
    endif
endin

instr SPACE_TEXT_CLEAR_TEST
    semspaceclear(stext)
    neighs:i[][], scores:i[] = semspacequerytxt(stext, mtext, "warm analog sound", 1)
    nscore:i = lenarray(scores)
    if (nscore == 0) then
        prints("[PASS] semspaceclear i-rate: query after clear returned no scores\n")
        done += 1
    elseif (scores[0] > $MIN_SCORE) then
        prints("[FAIL] semspaceclear i-rate: score after clear=%.9f\n", scores[0])
        fail += 1
    else
        prints("[PASS] semspaceclear i-rate: score after clear=%.9f\n", scores[0])
        done += 1
    endif
    turnoff
endin

instr SPACE_AUDIO_BUILD_LOAD_QUERY_TEST
    semspacebuild(maudio, $AUDIO_ESPC, $AUDIO_DIR)
    s:i = semspace(maudio, $AUDIO_ESPC)

    neighs:i[][], scores:i[] = semspacequeryaudio(s, maudio, $AUDIO_TEST_FILE, 2)
    if (s <= 0) then
        prints("[FAIL] semspacebuild audio + semspacequeryaudio: invalid handle=%d\n", s)
        fail += 1
        turnoff
    endif
    nscore:i = lenarray(scores)
    if (nscore == 0) then
        prints("[FAIL] semspacebuild audio + semspacequeryaudio: query returned no scores\n")
        fail += 1
        turnoff
    endif
    if (scores[0] <= $MIN_SCORE) then
        prints("[FAIL] semspacebuild audio + semspacequeryaudio: handle=%d score=%.9f\n", s, scores[0])
        fail += 1
        turnoff
    endif

    semspacesave(s, $AUDIO_ESPC_SAVE)
    prints("[PASS] semspacebuild audio + semspacequeryaudio + semspacesave: score=%.9f\n", scores[0])
    done += 1
    turnoff
endin

instr SPACE_AUDIO_ADD_I_TEST
    prints("[INFO] semspaceaddaudio i-rate: begin\n")
    semspaceaddaudio(saudio, maudio, $AUDIO_TEST_FILE)
    prints("[INFO] semspaceaddaudio i-rate: done\n")
    turnoff
endin

instr SPACE_AUDIO_QUERY_I_TEST
    prints("[INFO] semspacequeryaudio i-rate: begin\n")
    neighs:i[][], scores:i[] = semspacequeryaudio(saudio, maudio, $AUDIO_TEST_FILE, 1)
    nscore:i = lenarray(scores)
    if (nscore == 0) then
        prints("[FAIL] semspaceaddaudio + semspacequeryaudio i-rate: query returned no scores; audio space may be empty\n")
        fail += 1
        turnoff
    endif
    prints("[INFO] semspacequeryaudio i-rate: query done score=%.9f\n", scores[0])
    dim:i = semdim(maudio)
    isum:i = 0
    icol:i = 0
    while (icol < dim) do
        isum += neighs[0][icol] * neighs[0][icol]
        icol += 1
    od
    nnorm:i = sqrt(isum)
    prints("[INFO] semspacequeryaudio i-rate: norm done neigh_norm=%.9f\n", nnorm)

    if (scores[0] <= $MIN_SCORE) then
        prints("[FAIL] semspaceaddaudio + semspacequeryaudio i-rate: score=%.9f neigh_norm=%.9f\n", scores[0], nnorm)
        fail += 1
    else
        prints("[PASS] semspaceaddaudio + semspacequeryaudio i-rate: score=%.9f neigh_norm=%.9f\n", scores[0], nnorm)
        done += 1
    endif
    turnoff
endin

instr SPACE_AUDIO_K_ADD_TEST
    if (p3 < 0.25) then
        prints("[SKIP] semspaceaddaudio.k: instrument duration too short for trigger setup (p3=%.3f)\n", p3)
        skip += 1
        turnoff
    endif

    ktrig:k = (timeinsts() < 0.2 ? 1 : 0)
    semspaceaddaudio(saudio, maudio, $AUDIO_TEST_FILE, ktrig)

    if (timeinsts() > p3 - 0.1) then
        prints("[INFO] semspaceaddaudio.k submitted\n")
        turnoff
    endif
endin

instr SPACE_AUDIO_K_QUERY_TEST
    if (p3 < 2) then
        prints("[SKIP] semspacequeryaudio.k: instrument duration too short for async audio query (p3=%.3f)\n", p3)
        skip += 1
        turnoff
    endif

    neighs:k[][], scores:k[], gate:k = semspacequeryaudio(saudio, maudio, $AUDIO_TEST_FILE, 1)
    if (gate == 1 && scores[0] > $MIN_SCORE) then
        println("[PASS] semspaceaddaudio.k + semspacequeryaudio.k: score=%.9f", scores[0])
        done += 1
        turnoff
    endif

    if (timeinsts() > p3 - 0.1) then
        println("[FAIL] semspaceaddaudio.k + semspacequeryaudio.k: no valid score, gate=%d score=%.9f", gate, scores[0])
        fail += 1
        turnoff
    endif
endin

instr SPACE_AUDIO_FT_QUERY_I_TEST
    if (ftlen(audio_table) <= 0) then
        prints("[SKIP] semspacequeryaudioft i-rate: audio ftable is empty or invalid\n")
        skip += 1
        turnoff
    endif

    neighs:i[][], scores:i[] = semspacequeryaudioft(saudio, maudio, audio_table, 1, 0)
    nscore:i = lenarray(scores)
    if (nscore == 0) then
        prints("[FAIL] semspacequeryaudioft i-rate: query returned no scores; audio space may be empty\n")
        fail += 1
        turnoff
    endif
    dim:i = semdim(maudio)
    isum:i = 0
    icol:i = 0
    while (icol < dim) do
        isum += neighs[0][icol] * neighs[0][icol]
        icol += 1
    od
    nnorm:i = sqrt(isum)

    if (scores[0] <= $MIN_SCORE) then
        prints("[FAIL] semspacequeryaudioft i-rate: score=%.9f neigh_norm=%.9f\n", scores[0], nnorm)
        fail += 1
    else
        prints("[PASS] semspacequeryaudioft i-rate: score=%.9f neigh_norm=%.9f\n", scores[0], nnorm)
        done += 1
    endif
    turnoff
endin

instr SPACE_AUDIO_FT_QUERY_K_TEST
    if (p3 < 2.5) then
        prints("[SKIP] semspacequeryaudioft.k: duration too short for trigger at 2s (p3=%.3f)\n", p3)
        skip += 1
        turnoff
    endif
    if (ftlen(rec_table) <= 0) then
        prints("[SKIP] semspacequeryaudioft.k: capture ftable is empty or invalid\n")
        skip += 1
        turnoff
    endif

    sig:a = diskin2($AUDIO_TEST_FILE, 1, 0, 1)
    andx:a = phasor(sr / ftlen(rec_table))
    tablew(sig, andx, rec_table, 1)

    ktrig:k = (timeinsts() > 2 && timeinsts() < 2.2 ? 1 : 0)
    neighs:k[][], scores:k[], gate:k = semspacequeryaudioft(saudio, maudio, rec_table, 1, ktrig, 1)

    if (gate == 1 && scores[0] > $MIN_SCORE) then
        println("[PASS] semspacequeryaudioft.k: score=%.9f", scores[0])
        done += 1
        turnoff
    endif

    if (timeinsts() > p3 - 0.1) then
        println("[FAIL] semspacequeryaudioft.k: no valid score, gate=%d score=%.9f", gate, scores[0])
        fail += 1
        turnoff
    endif
endin

instr STT_SUBMIT_FILE_TEST
    semsttsubmitfile(mstt, $STT_AUDIO_TRACK)
    prints("[INFO] semsttsubmitfile queued\n")
    turnoff
endin

instr STT_POLL_FILE_TEST
    if (p3 < 5) then
        prints("[SKIP] semsttsubmitfile poll: duration too short for STT worker (p3=%.3f)\n", p3)
        skip += 1
        turnoff
    endif

    ready:k = semsttready(mstt)
    if (ready == 1) then
        text:S, len:k = semsttresult(mstt)
        if (len <= 0) then
            println("[FAIL] semsttsubmitfile/semsttready/semsttresult: empty result")
            fail += 1
        else
            println("[PASS] semsttsubmitfile/semsttready/semsttresult: len=%d text=%s", len, text)
            done += 1
        endif
        turnoff
    endif

    if (timeinsts() > p3 - 0.1) then
        println("[FAIL] semsttsubmitfile/semsttready/semsttresult: timeout")
        fail += 1
        turnoff
    endif
endin

instr STT_SUBMIT_ARRAY_TEST
    if (ftlen(stt_table) <= 0) then
        prints("[SKIP] semsttsubmitarray: STT ftable is empty or invalid\n")
        skip += 1
        turnoff
    endif

    samples:i[] = init(ftlen(stt_table))
    copyf2array(samples, stt_table)

    isum:i = 0
    ipeak:i = 0
    indx:i = 0
    while (indx < lenarray(samples)) do
        aval:i = abs(samples[indx])
        isum += samples[indx] * samples[indx]
        if (aval > ipeak) then
            ipeak = aval
        endif
        indx += 1
    od
    irms:i = sqrt(isum / lenarray(samples))

    semsttsubmitarray(mstt, samples)
    prints("[INFO] semsttsubmitarray queued: samples=%d rms=%.9f peak=%.9f\n", ftlen(stt_table), irms, ipeak)
    turnoff
endin

instr STT_POLL_ARRAY_TEST
    if (p3 < 5) then
        prints("[SKIP] semsttsubmitarray poll: duration too short for STT worker (p3=%.3f)\n", p3)
        skip += 1
        turnoff
    endif

    ready:k = semsttready(mstt)
    if (ready == 1) then
        text:S, len:k = semsttresult(mstt)
        if (len <= 0) then
            println("[FAIL] semsttsubmitarray/semsttresult: empty result")
            fail += 1
        else
            println("[PASS] semsttsubmitarray/semsttresult: len=%d text=%s", len, text)
            done += 1
        endif
        turnoff
    endif

    if (timeinsts() > p3 - 0.1) then
        println("[FAIL] semsttsubmitarray/semsttresult: timeout")
        fail += 1
        turnoff
    endif
endin

instr STT_SUBMIT_FT_TEST
    if (ftlen(stt_table) <= 0) then
        prints("[SKIP] semsttsubmitft: STT ftable is empty or invalid\n")
        skip += 1
        turnoff
    endif

    samples:i[] = init(ftlen(stt_table))
    copyf2array(samples, stt_table)

    isum:i = 0
    ipeak:i = 0
    indx:i = 0
    while (indx < lenarray(samples)) do
        aval:i = abs(samples[indx])
        isum += samples[indx] * samples[indx]
        if (aval > ipeak) then
            ipeak = aval
        endif
        indx += 1
    od
    irms:i = sqrt(isum / lenarray(samples))

    semsttsubmitft(mstt, stt_table)
    prints("[INFO] semsttsubmitft queued: ftlen=%d rms=%.9f peak=%.9f\n", ftlen(stt_table), irms, ipeak)
    turnoff
endin

instr STT_POLL_FT_TEST
    if (p3 < 5) then
        prints("[SKIP] semsttsubmitft poll: duration too short for STT worker (p3=%.3f)\n", p3)
        skip += 1
        turnoff
    endif

    ready:k = semsttready(mstt)
    if (ready == 1) then
        text:S, len:k = semsttresult(mstt)
        if (len <= 0) then
            println("[FAIL] semsttsubmitft/semsttresult: empty result")
            fail += 1
        else
            println("[PASS] semsttsubmitft/semsttresult: len=%d text=%s", len, text)
            done += 1
        endif
        turnoff
    endif

    if (timeinsts() > p3 - 0.1) then
        println("[FAIL] semsttsubmitft/semsttresult: timeout")
        fail += 1
        turnoff
    endif
endin

instr STT_LIVE_TEST
    if (p3 < 5) then
        prints("[SKIP] semsttsubmitlive: duration too short for live capture + STT worker (p3=%.3f)\n", p3)
        skip += 1
        turnoff
    endif

    sig:a = diskin2($STT_AUDIO_TRACK, 1, 0, 0)
    semsttsubmitlive(mstt, sig, 2)

    ready:k = semsttready(mstt)
    if (ready == 1) then
        text:S, len:k = semsttresult(mstt)
        if (len <= 0) then
            println("[INFO] semsttsubmitlive/semsttresult: skipped empty result")
        else
            println("[PASS] semsttsubmitlive/semsttresult: len=%d text=%s", len, text)
            done += 1
            turnoff
        endif
    endif

    if (timeinsts() > p3 - 0.1) then
        println("[FAIL] semsttsubmitlive/semsttresult: timeout")
        fail += 1
        turnoff
    endif
endin

instr RESULT
    println("\n[TEST RESULT]\nFAILED: %d\nSKIPPED: %d\nDONE: %d\n", fail, skip, done)
    turnoff
endin

</CsInstruments>
<CsScore>

i "LOAD_TEST"                         0    0.1
i "EMBED_TEXT_I_TEST"                 ^+1  0.1
i "EMBED_TEXT_FILE_TEST"              ^+1  0.1
i "EMBED_TEXT_K_TEST"                 ^+1  3
i "EMBED_AUDIO_FILE_TEST"             ^+1  0.1
i "EMBED_AUDIO_FT_TEST"               ^+1  0.1
i "EMBED_AUDIO_K_TEST"                ^+1  8
i "SPACE_TEXT_BUILD_LOAD_QUERY_TEST"  ^+1  0.1
i "SPACE_TEXT_ADD_I_TEST"             ^+1  0.1
i "SPACE_TEXT_QUERY_I_TEST"           ^+1  0.1
i "SPACE_TEXT_K_ADD_TEST"             ^+1  1
i "SPACE_TEXT_K_QUERY_TEST"           ^+1  5
i "SPACE_TEXT_CLEAR_TEST"             ^+1  0.1
i "SPACE_AUDIO_BUILD_LOAD_QUERY_TEST" ^+1  0.1
i "SPACE_AUDIO_ADD_I_TEST"            ^+1  0.1
i "SPACE_AUDIO_QUERY_I_TEST"          ^+1  0.1
i "SPACE_AUDIO_K_ADD_TEST"            ^+1  2
i "SPACE_AUDIO_K_QUERY_TEST"          ^+1  8
i "SPACE_AUDIO_FT_QUERY_I_TEST"       ^+1  0.1
i "SPACE_AUDIO_FT_QUERY_K_TEST"       ^+1  8
i "STT_SUBMIT_FILE_TEST"              ^+1  0.1
i "STT_POLL_FILE_TEST"                ^+1  50
i "STT_SUBMIT_ARRAY_TEST"             ^+1  0.1
i "STT_POLL_ARRAY_TEST"               ^+1  60
i "STT_SUBMIT_FT_TEST"                ^+1  0.1
i "STT_POLL_FT_TEST"                  ^+1  30
i "STT_LIVE_TEST"                     ^+1  30
i "RESULT"                            ^+1  0.1

</CsScore>
</CsoundSynthesizer>
