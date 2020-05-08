# Hamming Erasure Coding
Library for employing Hamming codes for erasure coding.

## Theory
Since each data bit in a Hamming code is covered by a unique pattern of parity bits, it should be possible to restore missing bits.  Since adding an overall parity bit allows for detecting three bit errors, when used as an erasure code it should allow for recovering up to three missing blocks.  
