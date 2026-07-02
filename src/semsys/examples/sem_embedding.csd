<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

#define MODEL_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/all-MiniLM-L6-v2/all-MiniLM-L6-v2-e2e" #

// load the end-to-end embedding model (dir must contain model.onnx + model.onnx.data)
handle@global:i = semload(256, $MODEL_DIR)

instr 1
    ldim:i = semdim(handle) // get latent dimension
    prints("Latent dimension size: %d\n", ldim)
    turnoff
endin

instr SENTENCE_K
    sentence:S = "sound synthesis in blue sky"
    // pool_embed = mean-pooled sentence vector; changed = 1 on the pass the text changes
    pool_embed:k[], changed:k = semembedtxt:k(handle, sentence)
    printarray(pool_embed, -1)
endin

instr SENTENCE_I
    sentence:S = "On a quiet autumn morning,                                                                         \
        Emma decided to take a different path to work.                                                               \
        Instead of following the busy streets, she wandered                                                          \
        through an old park she had never noticed before.                                                            \
        The trees were covered in golden leaves, and the air smelled fresh after the night’s rain.                   \
        As she walked, she discovered a small wooden bench with a notebook resting on it.                            \
        Curious, she opened the notebook and found that it was filled                                                \
        with short messages written by strangers. Some shared dreams they hoped to achieve,                          \
        while others wrote about lessons they had learned from difficult moments in life.                            \
        Inspired by their honesty, Emma added her own message: Never be afraid to start again.                       \
        Every new beginning brings a chance to become a better version of yourself.                                  \
        She closed the notebook, placed it back on the bench, and continued her walk with a smile.                   \
        For the rest of the day, she couldn’t stop thinking about the people who had written                         \
        in the notebook. Even though they had never met, their words had created a quiet connection                  \
        that reminded her how similar people can be, no matter where they come from. From that day on,               \
        Emma visited the park every Friday morning, hoping to leave another encouraging message for                  \
        the next person who might need it."

    pool_embed:i[][] = semembedtxt:i(handle, sentence)
    printarray(pool_embed)
endin

</CsInstruments>
<CsScore>

i 1 0 1
i "SENTENCE_K" 2 1
i "SENTENCE_I" 3 1

</CsScore>
</CsoundSynthesizer>
