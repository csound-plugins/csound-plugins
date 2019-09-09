jsusfx_csound - Jesusonic FX for Csound
=======================================
The jsusfx implementation is done through one main opcode called `jsfx` and a family of opcodes `jsfx_*` which 
allow more detailed control for the cases when this is needed.

jsfx
-----

`jsfx` is the runtime object to expose JSFX scripts in csound. It has the form

    ihandle, aout1 [, aout2, ...]  jsfx Spath, ain1 [, ain2, ...] [, ksliderid1, kval1, ksliderid2, kval2, ...]
     
The script will be searched first relative to the .csd file, then in the $SSDIR folder. Otherwise an absolute 
path is necessary. 


Version 0.4
-----------
* Multi-channel support
* Native ARM support (Raspberry Pi)
* Windows build support
* File API support
* @import support
* Midi in/out support
* More support of extended sliders
* Various bug fixes


Limitations
-----------
* @gfx, @serialize section is ignored