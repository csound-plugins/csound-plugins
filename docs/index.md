# Csound Plugins

Welcome to the csound-plugins wiki! 

This is a collection of plugins for [csound](https://csound.com/)

# Installation

See [installation](installation)


## klib

A hashtable for csound

* [dict_new](opcodes/dict_new): Create a hashtable 
* [dict_free](opcodes/dict_free): Free a hashtable 
* [dict_get](opcodes/dict_get): Get a value from a hashtable 
* [dict_set](opcodes/dict_set): Set (or remove) a value from a hashtable 
* [dict_size](opcodes/dict_size): Returns the number of key:value pairs in a dict 
* [dict_query](opcodes/dict_query): Query different properties of a dict 
* [dict_print](opcodes/dict_print): Prints the contents of a dict 


## poly

Multiple (parallel or sequential) instances of an opcode

* [poly](opcodes/poly): poly creates and controls multiple parallel version of an opcode 
* [polyseq](opcodes/polyseq): polyseq creates and controls multiple sequential version of an opcode 


## sched

Schedule an action when note is stopped

* [atstop](opcodes/atstop): Schedule an instrument at the end of the current instrument 



