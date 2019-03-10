# klib

These plugins implement a hashtable and all its operations

* A dict can be created by a note or by instr 0.
* lifetime: either tied to the lifetime of the note, or it can
  survive the note until dict_free is called later. When a dict
  is not tied to a note, its handle can be passed around.
  By default, a dict is local (is freed with the end of the note)
* key can be either a string or an integer
* value can be either be a string or a float (maybe in the future also audio
  and arrays?)

Operations implemented are: get, set, del, free, print, query 