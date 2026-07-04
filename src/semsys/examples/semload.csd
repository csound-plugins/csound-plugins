<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semload.csd
;
; semload initialises a semsys context from an end-to-end ONNX model directory
; (must contain model.onnx; keep model.onnx.data there too if present) and returns
; a handle used by every other semsys opcode.
;
;   text model  -> maxlen = token cap (e.g. 256 for all-MiniLM-L6-v2)
;   audio model -> maxlen = -1 ("full", no sequence cap; e.g. PANNs CNN14)
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define TEXT_DIR  # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/all-MiniLM-L6-v2/all-MiniLM-L6-v2-e2e" #
#define AUDIO_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/PANNs_CNN14_ONNX" #

h_txt@global:i = semload(256, $TEXT_DIR)   ; text model
h_aud@global:i = semload(-1, $AUDIO_DIR)   ; audio model, no sequence cap

instr 1
    prints("text dim = %d | audio dim = %d\n", semdim(h_txt), semdim(h_aud))
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
