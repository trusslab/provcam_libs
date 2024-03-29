# ProvCam: A Camera Module with Self-Contained TCB for Producing Verifiable Videos

:paperclip: [ProvCam Paper](https://doi.org/10.1145/3636534.3649383) 

:computer: [ProvCam Main Repository](https://github.com/trusslab/provcam)
This repo hosts the documentation of building and running ProvCam and other misc content. 

:computer: [ProvCam Hardware Repository](https://github.com/trusslab/provcam_hw)
This repo hosts ProvCam's hardware system design and its documentation.

:computer: [ProvCam Firmware Repository](https://github.com/trusslab/provcam_ctrl)
This repo hosts firmware running the microcontroller of ProvCam trusted camera module and its documentation.

:computer: [ProvCam OS Repository](https://github.com/trusslab/provcam_linux)
This repo hosts OS(a custom version of Petalinux) running on ProvCam's system and its documentation. 
Note that the OS represents the main camera OS, which is untrusted in ProvCam. 

:computer: [ProvCam Software Repository](https://github.com/trusslab/provcam_libs/tree/main)
This repo hosts some software and libraries running in the OS and their documentation.

Authors: \
[Yuxin (Myles) Liu](https://lab.donkeyandperi.net/~yuxinliu/) (UC Irvine)\
[Zhihao Yao](https://web.njit.edu/~zy8/) (NJIT)\
[Mingyi Chen](https://imcmy.me/) (UC Irvine)\
[Ardalan Amiri Sani](https://ics.uci.edu/~ardalan/) (UC Irvine)\
[Sharad Agarwal](https://sharadagarwal.net/) (Microsoft)\
[Gene Tsudik](https://ics.uci.edu/~gts/) (UC Irvine)

The work of UCI authors was supported in part by the NSF Awards #1763172, #1953932, #1956393, and #2247880 as well as NSA Awards #H98230-20-1-0345 and #H98230-22-1-0308.

We provide design/implmentation documentation and a step-by-step guide to recreate ProvCam's hardware and software prototype mentioned in our paper. 

---

## Table of Contents

- [ProvCam](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#provcam-a-camera-module-with-self-contained-tcb-for-producing-verifiable-videos)
    - [Table of Contents](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#table-of-contents)
    - [Hardware](https://github.com/trusslab/provcam_hw/tree/main/sources?tab=readme-ov-file#hardware)
        - [GENERAL_HASHER (sha256_core.v)](https://github.com/trusslab/provcam_hw/tree/main/sources?tab=readme-ov-file#general_hasher-sha256_corev)
        - [ISP_HASHER (r_hasher_4_isp.v)](https://github.com/trusslab/provcam_hw/tree/main/sources?tab=readme-ov-file#isp_hasher-r_hasher_4_ispv)
        - [ENCODER_HASHER (axixbar.v)](https://github.com/trusslab/provcam_hw/tree/main/sources?tab=readme-ov-file#encoder_hasher-axixbarv)
    - [Firmware](https://github.com/trusslab/provcam_ctrl/tree/main?tab=readme-ov-file#firmware)
    - [OS](https://github.com/trusslab/provcam_linux/tree/main?tab=readme-ov-file#os)
    - [Libraries](https://github.com/trusslab/provcam_libs/tree/main?tab=readme-ov-file#libraries)
    - [Build](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#build)
        - [System Requirements](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#system-requirements)
            - [Hardware](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#hardware)
            - [Xilinx Vivado and Vitis](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#xilinx-vivado-and-vitis)
            - [Xilinx Petalinux](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#xilinx-petalinux)
            - [Misc.](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#misc)
        - [Hadware Design](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#hadware-design)
        - [Firmware](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#firmware)
        - [OS](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#os)
    - [Run](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#run)
        - [Preparing the SD Card](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#preparing-the-sd-card)
        - [Hardware Preparation](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#hardware-preparation)
        - [Preparing the UART Consoles](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#preparing-the-uart-consoles)
        - [Preparing the Vitis Debug Environment](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#preparing-the-vitis-debug-environment)
        - [Running ProvCam](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#running-provcam)
    - [References](https://github.com/trusslab/provcam/tree/main?tab=readme-ov-file#references)

## Libraries

This repo hosts some software and libraries running in the OS. 
The libraries are used to interact with the hardware and the firmware. 
The main purpose of our modification is to enable ProvCam's added security features. 
