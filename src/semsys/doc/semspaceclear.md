# semspaceclear

## Abstract

Clear all vectors from an in-memory semantic space.

## Description

`semspaceclear` empties a [semspace](semspace.md) handle by setting its stored vector
count to zero. The space remains valid, keeps its embedding dimension, and can be populated
again with [semspaceaddtxt](semspaceaddtxt.md) or [semspaceaddaudio](semspaceaddaudio.md).

The allocated vector capacity is kept for reuse; the opcode does not free and recreate the
space. It also does not change or unload the embedding model handle.

The k-rate form clears on a **rising edge** of `ktrig` (`<= 0` to `> 0`). Holding `ktrig`
positive does not clear repeatedly.

No synchronization is performed with asynchronous k-rate queries already running on the
same space. For deterministic results, trigger `semspaceclear` when no pending query on
that space matters.

## Syntax

```csound
semspaceclear(space:i)
semspaceclear(space:i, trig:k)
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `trig:k`: clear on rising edge.

## Execution Time

* Init, or k-rate with trigger

## Examples

```csound
e_handle@global:i = semload(256, "path/to/model_dir")
s_handle@global:i = semspace(e_handle)

instr add
    semspaceaddtxt(s_handle, e_handle, "a warm analog texture")
    turnoff
endin

instr clear_once
    semspaceclear(s_handle)
    turnoff
endin

instr clear_on_trigger
    ktrig:k = metro(0.1)
    semspaceclear(s_handle, ktrig)
endin
```

## See also

* [semspace](semspace.md)
* [semspaceaddtxt](semspaceaddtxt.md)
* [semspaceaddaudio](semspaceaddaudio.md)
* [semspacesave](semspacesave.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
