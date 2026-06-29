<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 2
nchnls = 2
0dbfs = 1

#define EPS # pow(10, -12) #
#define MODEL_DIR # "/Users/pm/Desktop/all-MiniLM-L6-v2" #

seed(0)

// center frequencies for the partial bank (constant -> i-array)
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

// generate mask -> spare spectra avoid beating
opcode get_partials_mask(npart:i, dim:i):(i[])
    mask:i[] = init(npart)
    i:i = 0
    while (i < npart) do
        ndx:i = random(0, dim - 1)
        mask[i] = ndx
        i += 1
    od
    xout(mask)
endop

opcode osc_bank(amp:k[], freq:k[], dim:i, cnt:o):(a)
    sig:a = poscil(amp[cnt], freq[cnt]) * 0.81
    if (cnt < (dim - 1)) then
        rsig:a = osc_bank(amp, freq, dim, cnt + 1)
        xout(sig + rsig)
    else
        xout(sig)
    endif
endop

// load model dir (must contain model.onnx + tokenizer.onnx)
e_handle@global:i = semload(256, $MODEL_DIR)
sdim@global:i = semdim(e_handle)

// build a .espc space from a corpus once
semspacebuild(e_handle, "corpus.espc", "corpus.txt")
s@global:i = semspace(e_handle, "corpus.espc")

// turn a semantic query into a bank of partials, once
//
// idea: each embedding coordinate becomes the amplitude of a sine partial placed at a
// fixed center frequency. The query's MEANING shapes the timbre.
instr 1
    neighs:k[][], scores:k[] = semspacequery(s, "warm analog texture", 3)
    cfreqs:i[] = get_center_freqs(90, 2500, sdim)

    npart:i = 30
    mask:i[] = get_partials_mask(npart, sdim)
    printarray(mask)

    // neighs is k-rate (query runs at perf): build the bank ONCE, when it is ready
    amp:k[] = init(npart)
    frq:k[] = init(npart)
    kidx:k = 0
    while (kidx < npart) do
        endx:k = mask[kidx]
        amp[kidx] = abs(neighs[0][endx])   // amplitude = |embedding coordinate|
        frq[kidx] = cfreqs[endx]
        kidx += 1
    od

    // normalize so the amplitudes sum to 1 -> the summed partials peak at <= 1
    asum:k = sumarray(amp)
    amp = amp / (asum + $EPS)

    add_sig:a = osc_bank(amp, frq, npart) * expseg(0.001, p3 / 2, 1, p3 / 2, 0.001)
    out(add_sig, add_sig)
endin

</CsInstruments>
<CsScore>

i 1 0 10

</CsScore>
</CsoundSynthesizer>
