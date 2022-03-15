# Table Formats

Each character map table starts with a single nonzero byte, indicating the table's format.

## Extended ASCII

Format 1 is for "extended ASCII". Encoded values 0-127 are identical to ASCII, and encoded values 128-255 are mapped to single Unicode characters.

The table contains 128 entries, for encoded values 128-255, with the following format:

    u8    Length of Unicode character
    u8[]  Unicode character, UTF-8
    u8    Length of normalized Unicode character, may be zero
    u8[]  Unicode character in NFD normal form, UTF-8

The second copy of the character is only present if the character decomposes into multiple characters.
