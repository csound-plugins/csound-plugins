<CsoundSynthesizer>
<CsOptions>
-o dac3
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

#define FFT_SIZE # 1024 #
#define PLEN # 128 #
#define MODEL_DIR # "/Users/pm/Desktop/all-MiniLM-L6-v2" #


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

// load model dir (must contain model.onnx + tokenizer.onnx)
e_handle@global:i = semload(256, $MODEL_DIR)
sdim@global:i = semdim(e_handle)

// build a .espc space from a corpus once
semspacebuild(e_handle, "corpus.espc", "corpus.txt")

// impulse response table (filled by "BUILD_IR" instr, convolved by instr 2)
ir_table@global:i ftgen 0, 0, $FFT_SIZE, 2, 0

// build the static filter impulse response from a semantic query, once
instr BUILD_IR
    s:i = semspace(e_handle, "corpus.espc")
    neighs:i[][], scores:i[] = semspacequery(s, "space blue", 3)

    bin_freqs:i[] = rfft_bin_freqs() // nbins
    cfreqs:i[] = get_center_freqs(90, 1500, sdim)
    sigma:i = 250
    nbins:i = $FFT_SIZE / 2 + 1

    // neighs is k-rate (query runs at perf): build H/IR ONCE, at k-time, after it is ready
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

    // IR is ready -> start the synth voice
    eventi("i", 2, 0, 15)
    ; turnoff
endin

// partitioned-FFT convolution with the precomputed impulse response
instr 2
    sig:a = diskin2("/Users/pm/AcaHub/AudioSamples/vox.wav", 1)
    aout:a = ftconv(sig, ir_table, $PLEN)
    out(aout, aout)
endin

</CsInstruments>
<CsScore>

i "BUILD_IR" 0 0.1

</CsScore>
</CsoundSynthesizer>
