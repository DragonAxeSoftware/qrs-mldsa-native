[//]: # (SPDX-License-Identifier: CC-BY-4.0)

# Proofs for mldsa-native

This directory contains material related to the formal verification of the source code of mldsa-native.

## C verification: CBMC

We use the [C Bounded Model Checker (CBMC)](https://github.com/diffblue/cbmc) to show the absence of various classes of undefined behaviour in the mldsa-native C source, including out of bounds memory accesses and integer overflows. See [proofs/cbmc](cbmc), or the [proof_guide](https://github.com/pq-code-package/mlkem-native/blob/main/proofs/cbmc/proof_guide.md).
