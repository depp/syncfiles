# Character Conversion Tables

Used by SyncFiles.

This program generates the tables necessary to convert from UTF-8 to Mac OS Roman.

The conversion process is entirely table-driven. The table maps a (state, input) pair to a (state, output) pair. The initial state is 0. A transition to state 0 is considered invalid.

A transition may have _both_ a state and output. This means that the input may be translated in different ways depending on the bytes that follow. The translation code prefers the longest path through the state table that results in an output.

The table is compressed with PackBits to reduce its size by a factor of 22x.
