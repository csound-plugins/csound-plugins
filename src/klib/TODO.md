# klib TODO

## sderef / sview zero-copy alias (left as-is)

`sderef.i` / `sderef.k` (`sview_i`, `sview_init`/`sview_k` in
`src/klib/src/klib.c`) do NOT copy the string: they alias the string-cache
buffer directly (`outstr->data = ks->s`, no `Strdup`).

Consequences:

- The output `S` variable does not own its memory. Do not `free` / overwrite
  it manually.
- If the cache entry is removed (`cache_pop`) or the cache globals are reset
  while the view is still live, the view dangles.
- `sview_k` overwrites `p->outstr->data` without freeing the previous buffer;
  only use it on a fresh output variable (as Csound provides at init).

Deliberately left as-is for performance (avoids a copy per access). If this
ever becomes a problem, switch `sview_k` to `stringdat_set` (copy) or add a
refcount / validity check before aliasing.
