[//]: # (SPDX-License-Identifier: CC-BY-4.0)

# mldsa-native

![CI](https://github.com/pq-code-package/mldsa-native/actions/workflows/ci.yml/badge.svg)
![Benchmarks](https://github.com/pq-code-package/mldsa-native/actions/workflows/bench.yml/badge.svg)
[![License: Apache](https://img.shields.io/badge/license-Apache--2.0-green.svg)](https://www.apache.org/licenses/LICENSE-2.0)
[![License: ISC](https://img.shields.io/badge/License-ISC-blue.svg)](https://opensource.org/licenses/ISC)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

mldsa-native is a work-in-progress implementation of the ML-DSA[^FIPS204] post-quantum signature standard. It is a fork of the ML-DSA reference implementation[^REF].

The goal of mldsa-native is to be a secure, fast and portable C90 implementation of ML-DSA, paralleling [mlkem-native](https://github.com/pq-code-package/mlkem-native) for ML-KEM.

## Status

mldsa-native is work in progress. **WE DO NOT CURRENTLY RECOMMEND RELYING ON THIS LIBRARY IN A
PRODUCTION ENVIRONMENT OR TO PROTECT ANY SENSITIVE DATA.** Once we have the first stable version,
this notice will be removed.

## Quickstart for Ubuntu

```bash
# Install base packages
sudo apt-get update
sudo apt-get install make gcc python3 git

# Build and run tests using make
make build
make test

# The same using `tests`, a convenience wrapper around `make`
./scripts/tests all
# Show all options
./scripts/tests --help
```

## Prerequisite

Linux development environment or WSL.

Linux environment or WSL.
Conan V2
Python
C++ >= 11.4.0

## Development

Build using conan:

    conan install . --build=missing

Create package:
    
    conan create .

Test package (build and create the main package first and make sure the version is correct):

    ./scripts/build-run-tests.sh


## Push to private dev repository

    conan remote add qrs-conan-dev-local https://dragonaxe.jfrog.io/artifactory/api/conan/qrs-conan-dev-local

    conan remote login -p <token> qrs-conan-dev-local <username>

    conan upload qrscore/<optional version> -r qrs-conan-dev-local --confirm

## Formal Verification

We use the [C Bounded Model Checker (CBMC)](https://github.com/diffblue/cbmc) to prove absence of various classes of undefined behaviour in C, including out of bounds memory accesses and integer overflows. The proofs cover all C code in [mldsa/*](mldsa) and [mldsa/fips202/*](mldsa/fips202) involved in running mldsa-native with its C backend. See [proofs/cbmc](proofs/cbmc) for details.

## Security

All assembly in mldsa-native is constant-time in the sense that it is free of secret-dependent control flow, memory access,
and instructions that are commonly variable-time, thwarting most timing side channels. C code is hardened against compiler-introduced
timing side channels through suitable barriers and constant-time patterns.

## Design

mldsa-native is split into a _frontend_ and two _backends_ for arithmetic and FIPS202 / SHA3. The frontend is
fixed, written in C, and covers all routines that are not critical to performance. The backends are flexible, take care of
performance-sensitive routines, and can be implemented in C or native code (assembly/intrinsics); see
[mldsa/native/api.h](mldsa/native/api.h) for the arithmetic backend and 
[mldsa/fips202/native/api.h](mldsa/fips202/native/api.h) for the FIPS-202 backend. mldsa-native currently
offers backends for C, AArch64, and x86_64 - if you'd like contribute new backends, please reach out or just open a PR.

## Usage

Once mldsa-native reaches production readiness, you will be able to import [mldsa](mldsa) into your project's source tree and build using your preferred build system. The build system provided in this repository is for development purposes only.

### Will I be able to bring my own FIPS-202?

mldsa-native relies on and comes with an implementation of FIPS-202[^FIPS202]. If your library has its own FIPS-202 implementation, you
can use it instead of the one shipped with mldsa-native.

### Will I need to use the assembly backends?

No. If you want a C-only build, you will be able to omit the native backend directories from your import and configure the build accordingly in your config.h.

### Do I need to setup CBMC to use mldsa-native?

No. While we recommend considering formal verification for security-critical applications, mldsa-native will build and run without CBMC. Function contracts and loop invariants will be ignored unless `CBMC` is defined during compilation.

### Does mldsa-native support all security levels of ML-DSA?

Yes. mldsa-native supports all three ML-DSA security levels (ML-DSA-44, ML-DSA-65, ML-DSA-87) as defined in FIPS 204. The security level is a compile-time parameter.

### Does mldsa-native use hedged or deterministic signing?

By default, mldsa-native uses the "hedged" signing variant as specified in FIPS 204 Section 3.4, with `MLD_RANDOMIZED_SIGNING` enabled in [mldsa/config.h](mldsa/config.h). The hedged variant uses both fresh randomness at signing time and precomputed randomness from the private key. This helps mitigate fault injection attacks and side-channel attacks while protecting against potential flaws in the random number generator.

The deterministic variant can be enabled by undefining `MLD_RANDOMIZED_SIGNING`, but FIPS 204 warns that this should not be used on platforms where fault injection attacks and side-channel attacks are a concern, as the lack of fresh randomness makes fault attacks more difficult to mitigate.

### Does mldsa-native support the pre-hash/digest sign/verify mode (external mu)?

Yes. mldsa-native supports external mu mode, which allows for pre-hashing of messages before signing. This addresses the pre-hashing capability described in the NIST PQC FAQ[^NIST_FAQ] and detailed in NIST's guidance on FIPS 204 Section 6[^NIST_FIPS204_SEC6].

External mu mode enables applications to compute the message digest (mu) externally and provide it to the signing implementation, which is particularly useful for large messages or streaming applications where the entire message cannot be held in memory during signing.

### Does mldsa-native support HashML-DSA?

No. mldsa-native does not currently implement HashML-DSA, the hash-based variant of ML-DSA defined in FIPS 204. The current implementation focuses on the standard ML-DSA signature scheme.

### Will I be able to bring my own backend?

Yes, you will be able to add custom backends for ML-DSA native arithmetic and/or for FIPS-202. You will be able to follow the existing backends as templates.

## Have a Question?

If you think you have found a security bug in mldsa-native, please report the vulnerability through
Github's [private vulnerability reporting](https://github.com/pq-code-package/mldsa-native/security). 
Please do **not** create a public GitHub issue.

If you have any other question / non-security related issue / feature request, please open a GitHub issue.

## Contributing

If you want to help us build mldsa-native, please reach out. You can contact the mldsa-native team
through the [PQCA Discord](https://discord.com/invite/xyVnwzfg5R).

<!--- bibliography --->
[^FIPS204]: National Institute of Standards and Technology: FIPS 204 Module-Lattice-Based Digital Signature Standard, [https://csrc.nist.gov/pubs/fips/204/final](https://csrc.nist.gov/pubs/fips/204/final)
[^REF]: Bai, Ducas, Kiltz, Lepoint, Lyubashevsky, Schwabe, Seiler, Stehlé: CRYSTALS-Dilithium reference implementation, [https://github.com/pq-crystals/dilithium](https://github.com/pq-crystals/dilithium)
[^NIST_FAQ]: National Institute of Standards and Technology: Post-Quantum Cryptography FAQs, [https://csrc.nist.gov/Projects/post-quantum-cryptography/faqs#Rdc7](https://csrc.nist.gov/Projects/post-quantum-cryptography/faqs#Rdc7)
[^NIST_FIPS204_SEC6]: National Institute of Standards and Technology: FIPS 204 Section 6 Guidance, [https://csrc.nist.gov/csrc/media/Projects/post-quantum-cryptography/documents/faq/fips204-sec6-03192025.pdf](https://csrc.nist.gov/csrc/media/Projects/post-quantum-cryptography/documents/faq/fips204-sec6-03192025.pdf)
