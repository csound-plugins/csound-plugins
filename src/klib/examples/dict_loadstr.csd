<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

/*
  # Example file for dict_loadstr

  ## dict_loadstr

    idict dict_loadstr Sdictdef

  Create a new dict from a string definition of the form
  "keyA: valueA, keyB: valueB, ..."

  The dict creates has the type str:any, which means that the 
  keys are always strings and the values can be either numbers
  or strings. Keys do not need to be quoted. Values only need to
  be quoted if a string value consists of only numbers or includes
  itself quotation marks. Quotations must be single quotations: `'`

  The motivation behind this opcode is to be able to save a dictionary
  to a string, for example in the metadata of a soundfile, and be
  able to retrieve that as a dictionary inside csound
  
*/
0dbfs = 1


instr example1
  idict dict_loadstr "foo: 10,  bar :'barvalue', \nbaz: unquoted string"
  dict_print idict
  turnoff
endin

instr example2
  ; Read the metadata of a .mtx first
  Scomment sfreadmeta "test.mtx", "comment"
  if strlen(Scomment) == 0 then
    initerror "The .mtx file has no metadata"
  endif
  ; Parse the metadata as a dict
  idict dict_loadstr Scomment
  dict_print idict
  
  ; Now we load the data itself. The mtx format consists of a flat array of floats
  ; where the first numbers include the dimensions of the matrix saved. 
  ; As the columns metadata should show, the mtx file has a header:
  ; HeaderSize NumRows NumColumns [optionally other values] <The matrix as a flat array>
  ; The same header is duplicated in the metadata itself, so we can use that directly
  ; to make the code more self-documenting.
  itab ftgen 0, 0, 0, -1, "test.mtx", 0, 0, 0

  ; If the data is needed as an array it is possible to create an alias via memview.
  ; Otherwise use tab2array to create a copy
  iArr[] memview itab, dict_get:i(idict, "HeaderSize")
  ; iArr[] tab2array itab, dict_get:i(idict, "HeaderSize")

  ; Now reshape the array to recreate the original matrix
  reshapearray iArr, dict_get:i(idict, "NumRows"), dict_get:i(idict, "NumColumns")
  printarray iArr
  turnoff
endin
    
</CsInstruments>

<CsScore>
i "example2" 0 0.1
</CsScore>
</CsoundSynthesizer>