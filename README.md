# OP-TEE Trusted OS
This git contains source code for the secure side implementation of OP-TEE
project.

All official OP-TEE documentation has moved to http://optee.readthedocs.io.

// OP-TEE core maintainers


# Overview

This modified version of OP-TEE OS contains a proof-of-concept implementation of a proposed GlobalPlatform API compatible mechanism to collect CSI samples from a connected hardware peripheral and is part of the [CROSSCON project](https://crosscon.eu/). This mechanism is added as a Pseudo Trusted Application to OP-TEE's core, and can be implemented in a hardware-specific manner. As a consequence, Trusted Applications are independent of the actual hardware. The implementation is part of deliverable D4.4.

A demonstrator which uses this API in a Trusted Service used for Context-based Authentication and adds a working system around this TEE can be found in [this repository](https://github.com/crosscon/context-based-auth-crosscon-demo).

This specific implementation targets the Raspberry Pi 4 with its Wi-Fi peripheral, and runs as a VM on a CROSSCON Hypervisor instance. While there is no way to access CSI data by default, a firmware modification exists which forwards the samples to user space. This modification requires Linux to run, so we host a [separate VM with this modification](https://github.com/crosscon/context-based-auth-nexmon-vm) using the hypervisor which can only communicate with OP-TEE. The communication is facilitated through a Pseudo Trusted Application added the OP-TEE OS core at `core/pta/memread.c`, which also proivdes the API to the trusted application. The demonstrator contains more details for how exactly the setup is built.

A more detailed overview of how exactly the communication is handled and which exact API is used can be found in deliverable D4.3.


# Modifications for Context-based Authentication (CBA)

This repository is modified in order to support the CBA TA. These modifications serve two purposes:

- enable communication via mTLS (adapted configuration for libmbedtls)
- support communication with Nexmon VM (added PTA for reading from & writing to memory)

This OS is supposed to work together with the [accompanying Trusted Application (TA)](https://github.com/crosscon/context-based-auth-trusted-application). See there for more information on how to include this in a setup.


## Configuration

When integrating the Nexmon VM, the shared "physical" address space (which is mapped to OP-TEE's virtual memory) must be configured in `core/pta/csi.c` (both the start address and the address space size).

## Acknowledgments

The work presented in this repository is part of the
[CROSSCON project](https://crosscon.eu/) that received funding from the European
Union’s Horizon Europe research and innovation programme under grant agreement
No 101070537.

<p align="center">
    <img src="https://crosscon.eu/sites/crosscon/themes/crosscon/images/eu.svg" width=10% height=10%>
</p>

<p align="center">
    <img src="https://crosscon.eu/sites/crosscon/files/public/styles/large_1080_/public/content-images/media/2023/crosscon_logo.png?itok=LUH3ejzO" width=25% height=25%>
</p>
