# OP-TEE Trusted OS
This git contains source code for the secure side implementation of OP-TEE
project.

All official OP-TEE documentation has moved to http://optee.readthedocs.io.

// OP-TEE core maintainers


# Overview

This modified version of OP-TEE OS contains a proof-of-concept implementation of a proposed GlobalPlatform API compatible mechanism to collect CSI samples from a connected hardware peripheral and is part of the [CROSSCON project](https://crosscon.eu/). This mechanism is added as a Pseudo Trusted Application to OP-TEE's core, and can be implemented in a hardware-specific manner. As a consequence, Trusted Applications are independent of the actual hardware. The implementation is part of deliverable D4.4.

A demonstrator can be found in [this repository](https://github.com/crosscon/context-based-auth-crosscon-demo).


# Modifications for Context-based Authentication (CBA)

This repository is modified in order to support the CBA TA. These modifications serve two purposes:

- enable communication via mTLS (adapted configuration for libmbedtls)
- support communication with Nexmon VM (added PTA for reading from & writing to memory)

This OS is supposed to work together with the [accompanying Trusted Application (TA)](https://github.com/crosscon/context-based-auth-trusted-application). See there for more information on how to include this in a setup.


## Configuration

When integrating the Nexmon VM, the shared "physical" address space (which is mapped to OP-TEE's virtual memory) must be configured in `core/pta/csi.c` (both the start address and the address space size).
