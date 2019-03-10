# poly

This file implements the opcode 'poly', which enables to create and
control multiple versions of the same opcode, each with its state,
inputs and outputs.

In general, each opcode has a signature, given by the number and types of its
output and input arguments. For example, the opcode "oscili", as used
like "aout oscili kamp, kfreq", has a signature a/kk (a as output, kk as input)

To follow the example, to create 10 parallel versions of this opcode, it is
possible to use poly like this:


    kFreqs[] fillarray ...
    aOut[] poly 10, "oscili", 0.1, kFreqs[]


* kFreqs holds the frequencies of the oscillators. Changing its contents will
  modify the frequency of the oscillators.
* For each input argument it possible to define either an array of sufficient size
  (size >= num instances) or a scalar value, which will be used for all the
  instances of the opcode
* default arguments are handled in the same way as when calling the opcode directly