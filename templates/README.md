These are template/example plugin libraries to create plugins for csound. 
For c++ there are two different plugin frameworks, OpcodeBase.h (older) and
plugin.h (newer). There a two example plugins, one for each of the frameworks.

To create a new plugin library, copy one of the templates here to `src`

cp -r template-c ../src

src/
    poly/
    klib/
    ...
    myplugin/
    