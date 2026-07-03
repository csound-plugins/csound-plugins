<CsoundSynthesizer>
<CsOptions>
-o dac2
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

#define STT_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/whisper-core/model_e2e" # ; end-to-end STT model dir: must contain model.onnx; keep model.onnx.data there too if present
#define AUDIO # "/Users/pm/AcaHub/AudioSamples/spoken.wav" #
#define MODEL_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/all-MiniLM-L6-v2/all-MiniLM-L6-v2-e2e" # ; end-to-end embedding model dir: must contain model.onnx; keep model.onnx.data there too if present
#define FFT_SIZE # 1024 #
#define PLEN # 128 #

// center frequencies for the gaussian filterbank (constant -> i-array)
opcode get_center_freqs(minf:i, maxf:i, nfreqs:i):(i[])
    f:i[] = init(nfreqs)
    step:i = (maxf - minf) / nfreqs
    idx:i = 0
    while (idx < nfreqs) do
        f[idx] = minf + step * idx
        idx += 1
    od
    xout(f)
endop

// frequencies of the rfft bins: nbins = fft_size/2 + 1, f[i] = i*sr/fft_size
opcode rfft_bin_freqs():(i[])
    nbins:i = $FFT_SIZE / 2 + 1
    f:i[] = init(nbins)
    idx:i = 0
    while (idx < nbins) do
        f[idx] = idx * sr / $FFT_SIZE
        idx += 1
    od
    xout(f)
endop

// load handles
e_handle@global:i = semload(256, $MODEL_DIR)
sdim@global:i = semdim(e_handle)
h@global:i = semsttload($STT_DIR, 448, 4)
// build a .espc space from a corpus once, then load it into RAM
semspacebuild(e_handle, "corpus.espc", "corpus.txt")
s_handle@global:i = semspace(e_handle, "corpus.espc")

// impulse response table (filled by BUILD_IR, convolved by APPLY_IR)
ir_table@global:i ftgen 0, 0, $FFT_SIZE, 2, 0

; submit the file once at init; transcription runs on the worker thread
instr SUBMIT
    semsttsubmitfile(h, $AUDIO)
endin

; poll until the transcription is ready; hand the text to BUILD_IR and stop.
; the query/IR build must run once the text actually exists, so it lives in a
; separate instrument scheduled here (i-rate code in this k-block would otherwise
; init at t=0, with an empty query).
instr POLL
    r:k = semsttready(h)
    if (r == 1) then
        text:S, tlen:k = semsttresult(h)
        println("TRANSCRIPTION: %s\n", text)
        if (tlen > 0) then
            chnset(text, "sem_query") ; hand the text off via a channel (k-rate safe)
            event("i", "BUILD_IR", 0, 0.1)
        endif
        turnoff
    endif
endin

; build the impulse response from the query embedding, then trigger the convolution.
; runs once at its own init, when the "sem_query" channel already holds the transcription.
instr BUILD_IR
    query:S = chnget:S("sem_query")
    neighs:i[][], scores:i[] = semspacequerytxt(s_handle, e_handle, query, 3)

    bin_freqs:i[] = rfft_bin_freqs() // nbins
    cfreqs:i[] = get_center_freqs(90, 1500, sdim)
    sigma:i = 250
    nbins:i = $FFT_SIZE / 2 + 1

    H:i[] = init(nbins) // in f-domain
    idx:i = 0
    while (idx < sdim) do
        coeff:i = neighs[1][idx]
        gaussian:i[] = exp(-0.5 * ((bin_freqs - cfreqs[idx]) / sigma) ^ 2)
        H = H + (coeff * gaussian)
        idx += 1
    od

    // zero-phase magnitude spectrum -> real impulse response
    spec:i[] = r2c(H) // |.| = H, phase = 0
    ir:i[] = rifft(spec) // length = fft_size
    copya2ftab(ir, ir_table)
    eventi("i", "APPLY_IR", 0, 15)
    turnoff
endin

// partitioned-FFT convolution with the precomputed impulse response
instr APPLY_IR
    sig:a = diskin2("/Users/pm/AcaHub/AudioSamples/vox.wav", 1)
    aout:a = ftconv(sig, ir_table, $PLEN)
    out(aout, aout)
endin

</CsInstruments>
<CsScore>
i "SUBMIT" 0 10
i "POLL"   0 30
</CsScore>
</CsoundSynthesizer>
