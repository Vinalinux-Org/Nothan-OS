# 5 Graphics Accelerator (SGX)

This chapter describes the graphics accelerator for the device.

| Topic |                        | Page |
|-------|------------------------|------|
| 5.1   | Integration            | 526  |
| 5.2   | Functional Description | 528  |

[www.ti.com](http://www.ti.com)

## **Introduction**

This chapter describes the 2D/3D graphics accelerator (SGX) for the device.

**NOTE:** The SGX subsystem is a Texas Instruments instantiation of the POWERVR® SGX530 core from Imagination Technologies Ltd.

This document contains materials that are ©2003-2007 Imagination Technologies Ltd.

POWERVR® and USSE™ are trademarks or registered trademarks of Imagination Technologies Ltd.

The 2D/3D graphics accelerator (SGX) subsystem accelerates 2-dimensional (2D) and 3-dimensional (3D) graphics applications. The SGX subsystem is based on the POWERVR® SGX core from Imagination Technologies. SGX is a new generation of programmable POWERVR graphic cores. The POWERVR SGX530 v1.2.5 architecture is scalable and can target market segments, from portable devices to HMI.

### *5.0.10 POWERVR SGX Main Features*

- 2D and 3D graphics
- Tile-based architecture
- Universal scalable shader engine (USSE™) multithreaded engine incorporating pixel and vertex shader functionality
- Advanced shader feature set: in excess of OpenGL2.0
- Industry-standard API support: OpenGL ES 1.1 and 2.0, OpenVG v1.0.1
- Fine-grained task switching, load balancing, and power management
- Advanced geometry direct memory access (DMA) driven operation for minimum CPU interaction
- Programmable high-quality image anti-aliasing
- POWERVR SGX core MMU for address translation from the core virtual address to the external physical address (up to 4GB address range)
- Fully virtualized memory addressing for OS operation in a unified memory architecture
- Advanced and standard 2D operations [e.g., vector graphics, BLTs (block level transfers), ROPs (raster operations)]
- 32K stride support

### *5.0.11 SGX 3D Features*

- Deferred pixel shading
- On-chip tile floating point depth buffer
- 8-bit stencil with on-chip tile stencil buffer
- 8 parallel depth/stencil tests per clock
- Scissor test
- Texture support:
  - Cube map
  - Projected textures
  - 2D textures
  - Nonsquare textures
- Texture formats:
  - RGBA 8888, 565, 1555
  - Monochromatic 8, 16, 16f, 32f, 32int
  - Dual channel, 8:8, 16:16, 16f:16f
  - Compressed textures PVR-TC1, PVR-TC2, ETC1
  - Programmable support for YUV 420 and 422 formats for YUV/RGB color conversion
- Resolution support:
  - Frame buffer maximum size = 2048 x 2048
  - Texture maximum size = 2048 x 2048
- Texture filtering:
  - Bilinear, trilinear, anisotropic
  - Independent minimum and maximum control
- Antialiasing:
  - 4x multisampling
  - Up to 16x full scene anti-aliasing
  - Programmable sample positions
- Indexed primitive list support
  - Bus mastered
- Programmable vertex DMA
- Render to texture:
  - Including twiddled formats
  - Auto MipMap generation
- Multiple on-chip render targets (MRT).

**Note**: Performance is limited when the on-chip memory is not available.

[www.ti.com](http://www.ti.com)

### *5.0.12 Universal Scalable Shader Engine (USSE) – Key Features*

The USSE is the engine core of the POWERVR SGX architecture and supports a broad range of instructions.

- Single programming model:
  - Multithreaded with 16 simultaneous execution threads and up to 64 simultaneous data instances
  - Zero-cost swapping in, and out, of threads
  - Cached program execution model
  - Dedicated pixel processing instructions
  - Dedicated video encode/decode instructions
- SIMD execution unit supporting operations in:
  - 32-bit IEEE float
  - 2-way 16-bit fixed point
  - 4-way 8-bit integer
  - 32-bit bit-wise (logical only)
- Static and dynamic flow control:
  - Subroutine calls
  - Loops
  - Conditional branches
  - Zero-cost instruction predication
- Procedural geometry:
  - Allows generation of primitives
  - Effective geometry compression
  - High-order surface support
- External data access:
  - Permits reads from main memory using cache
  - Permits writes to main memory
  - Data fence facility
  - Dependent texture reads

### *5.0.13 Unsupported Features*

There are no unsupported SGX530 features for this device.

## **5.1 Integration**

```
+-------------------------------------------------------------------------------------+
|                                      (SoC)                                          |
|                                                                                     |
|                             L3 Fast Interconnect                                    |
|                    <------------------------------------------------>               |
|                             L3 Fast Interconnect                                    |
|                    <------------------------------------------------>               |
|                                                                                     |
|   +--------------------+                                     +-------------------+  |
|   |   MPU Subsystem    |------------------------------------>|   GFX Subsystem   |  |
|   +--------------------+                                     |                   |  |
|                                                              |  Master           |  |
|                                                              |  Slave            |  |
|                                                              |  THALIAIRQ        |  |
|                                                              |                   |  |
|                                                              |  SYSCLK           |  |
|                                                              |  MEMCLK           |  |
|                                                              |  CORECLK          |  |
|                                                              +-------------------+  |
|                                                                                     |
|                                                                                     |
|   CORE_CLKOUTM4 (200 MHz)                                                           |
|          |                                                                          |
|          v                                                                          |
|   PER_CLKOUTM2 (192 MHz)                                                            |
|          |                                                                          |
|   +--------------------+                                                            |
|   |        PRCM        |                                                            |
|   |                    |                                                            |
|   |  clock mux 0 / 1   |<-- CORE_CLKOUTM4 / PER_CLKOUTM2                            |
|   |        |           |                                                            |
|   |       /1, /2       |                                                            |
|   +--------------------+                                                            |
|          |                               |                                          |
|          | pd_gfx_gfx_fclk               | pd_gfx_gfx_l3_gclk                       |
|          v                               v                                          |
|      (to GFX CORECLK/MEMCLK)         (to GFX L3 clock)                              |
+-------------------------------------------------------------------------------------+
```

**SGX530 Integration**

### *5.1.1 SGX530 Connectivity Attributes*

The general connectivity attributes of the SGX530 are shown in the following table.

**Table 5-1. SGX530 Connectivity Attributes**

| Attributes          | Type                                |  |
|---------------------|-------------------------------------|--|
| Power domain        | GFX Domain                          |  |
| Clock domain        | SGX_CLK                             |  |
| Reset signals       | SGX_RST                             |  |
| Idle/Wakeup signals | Smart Idle<br>Initiator Standby     |  |
| Interrupt request   | THALIAIRQ (GFXINT) to MPU Subsystem |  |
| DMA request         | None                                |  |
| Physical address    | L3 Fast slave port                  |  |

### *5.1.2 SGX530 Clock and Reset Management*

The SGX530 uses separate functional and interface clocks. The SYSCLK is the clock for the slave interface and runs at the L3F frequency. The MEMCLK is the clock for the memories and master interface and also runs at the L3F frequency. The CORECLK is the functional clock. It can be sourced from either the L3F clock (CORE\_CLKOUTM4) or from the 192 MHz PER\_CLKOUTM2 and can optionally be divided by 2.

**Table 5-2. SGX530 Clock Signals**

| Clock signal                | Max Freq | Reference / Source               | Comments                        |
|-----------------------------|----------|----------------------------------|---------------------------------|
| SYSCLK<br>Interface clock   | 200 MHz  | CORE_CLKOUTM4                    | pd_gfx_gfx_l3_gclk<br>From PRCM |
| MEMCLK<br>Memory Clock      | 200 MHz  | CORE_CLKOUTM4                    | pd_gfx_gfx_l3_gclk<br>From PRCM |
| CORECLK<br>Functional clock | 200 MHz  | PER_CLKOUTM2 or<br>CORE_CLKOUTM4 | pd_gfx_gfx_fclk<br>From PRCM    |

### *5.1.3 SGX530 Pin List*

The SGX530 module does not include any external interface pins.

## **5.2 Functional Description**

### *5.2.1 SGX Block Diagram*

The SGX subsystem is based on the POWERVR® SGX530 core from Imagination Technologies. The architecture uses programmable and hard coded pipelines to perform various processing tasks required in 2D, 3D, and video processing. The SGX architecture comprises the following elements:

- Coarse grain scheduler
  - Programmable data sequencer (PDS)
  - Data master selector (DMS)
- Vertex data master (VDM)
- Pixel data master (PDM)
- General-purpose data master
- USSE
- Tiling coprocessor
- Pixel coprocessor
- Texturing coprocessor
- Multilevel cache

[Figure](#page-127-1) 5-1 shows a block diagram of the SGX cores.

```
+----------------------------------------------------------------------------------------+
|                                POWERVR SGX530 GFX Core                                 |
+----------------------------------------------------------------------------------------+
|                                                                                        |
|  +-------------------------+        +-------------------------+                        |
|  |   Vertex data master    |------->|                         |                        |
|  +-------------------------+        |  Coarse-grain scheduler |                        |
|                                     |  +-------------------+  |                        |
|  +-------------------------+        |  | Prog. data        |  |                        |
|  |    Pixel data master    |------->|  |   sequencer       |  |                        |
|  +-------------------------+        |  +-------------------+  |                        |
|                                     |  +-------------------+  |                        |
|  +-------------------------+        |  | Data master       |  |                        |
|  | General-purpose         |------->|  |   selector        |  |                        |
|  |   data master           |        |  +-------------------+  |                        |
|  +-------------------------+        +-------------------------+                        |
|                                                |                                       |
|                                                v                                       |
|                                 +-------------------------------+                      |
|                                 | Universal scalable shader     |                      |
|                                 | engine (USSE)                 |                      |
|                                 +-------------------------------+                      |
|                                    ^          ^           ^                            |
|                                    |          |           |                            |
|                   +----------------+          |           +----------------+           |
|                   |                           |                            |           |
|   +-----------------------------+   +------------------------+   +------------------+  |
|   |   Texturing coprocessor     |<->|   Multilevel cache     |<->| Pixel coprocessor|  |
|   +-----------------------------+   +------------------------+   +------------------+  |
|                                    |                                         ^         |
|                                    |                                         |         |
|                                    +------------------------------+          |         |
|                                                                   |          |         |
|                                                  +----------------+--------+ |         |
|                                                  |   Tiling coprocessor   |<+          |
|                                                  +------------------------+            |
|                                                                                        |
|  +---------------------------+                                                         |
|  | Power management control  |                                                         |
|  |      register block       |                                                         |
|  +---------------------------+                                                         |
|              ^                                                                         |
|              |                                                                         |
|  +-----------+-----------+          +-------------------+                              |
|  |    SOCIF           |  |          |        BIF        |                              |
|  +--------------------+  |          +-------------------+                              |
|          ^               |                  ^                                          |
|          |               |                  |                                          |
|    L3 interconnect  <----+---------- L3 interconnect ----------------------------------|
+----------------------------------------------------------------------------------------+
```

**Figure 5-1. SGX Block Diagram**

### *5.2.2 SGX Elements Description*

The coarse grain scheduler (CGS) is the main system controller for the POWERVR SGX architecture. It consists of two stages, the DMS and the PDS. The DMS processes requests from the data masters and determines which tasks can be executed given the resource requirements. The PDS then controls the loading and processing of data on the USSE.

There are three data masters in the SGX core:

• The VDM is the initiator of transform and lighting processing within the system. The VDM reads an

input control stream, which contains triangle index data and state data. The state data indicates the PDS program, size of the vertices, and the amount of USSE output buffer resource available to the VDM. The triangle data is parsed to determine unique indices that must be processed by the USSE. These are grouped together according to the configuration provided by the driver and presented to the DMS.

- The PDM is the initiator of rasterization processing within the system. Each pixel pipeline processes pixels for a different half of a given tile, which allows for optimum efficiency within each pipe due to locality of data. It determines the amount of resource required within the USSE for each task. It merges this with the state address and issues a request to the DMS for execution on the USSE.
- The general-purpose data master responds to events within the system (such as end of a pass of triangles from the ISP, end of a tile from the ISP, end of render, or parameter stream breakpoint event). Each event causes either an interrupt to the host or synchronized execution of a program on the PDS. The program may, or may not cause a subsequent task to be executed on the USSE.

The USSE is a user-programmable processing unit. Although general in nature, its instructions and features are optimized for three types of task: processing vertices (vertex shading), processing pixels (pixel shading), and video/imaging processing.

The multilevel cache is a 2-level cache consisting of two modules: the main cache and the mux/arbiter/demux/decompression unit (MADD). The MADD is a wrapper around the main cache module designed to manage and format requests to and from the cache, as well as providing Level 0 caching for texture and USSE requests. The MADD can accept requests from the PDS, USSE, and texture address generator modules. Arbitration, as well as any required texture decompression, are performed between the three data streams.

The texturing coprocessor performs texture address generation and formatting of texture data. It receives requests from either the iterators or USSE modules and translates these into requests in the multilevel cache. Data returned from the cache are then formatted according to the texture format selected, and sent to the USSE for pixel-shading operations.

To process pixels in a tiled manner, the screen is divided into tiles and arranged as groups of tiles by the tiling coprocessor. An inherent advantage of tiling architecture is that a large amount of vertex data can be rejected at this stage, thus reducing the memory storage requirements and the amount of pixel processing to be performed.

The pixel coprocessor is the final stage of the pixel-processing pipeline and controls the format of the final pixel data sent to the memory. It supplies the USSE with an address into the output buffer and then USSE returns the relevant pixel data. The address order is determined by the frame buffer mode. The pixel coprocessor contains a dithering and packing function.
