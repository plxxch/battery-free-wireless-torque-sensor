# Battery-Free Wireless Torque Sensor (BWST) — Demo Firmware

Firmware for a battery-free, wireless torque-sensing platform built around the
Texas Instruments **CC430F5137** (MSP430 CPU core plus a sub-1 GHz
CC1101-compatible radio). The repository contains two Code Composer Studio
(CCS) projects:

| Project | Role |
|---|---|
| `BWST-sensor-demo/` | Sensor node — samples a strain-gauge bridge and transmits the reading over the air. |
| `BWTS-Receiver-demo/` | Receiver node — receives the packet and streams the value over UART. |

> This is a research prototype that accompanies a paper. It is not certified
> for clinical, automotive, or industrial use.

## How it works

### Sensor node (`BWST-sensor-demo`)

- Digitises the strain-gauge bridge on **ADC12 channel A0**.
- Excites the bridge from **P2.2** only for the duration of a measurement
  (`P2OUT |= BIT2` … `P2OUT &= ~BIT2`), keeping the average power low.
- Uses the internal reference selected by `REFVSEL_0` (1.5 V); the result is
  12-bit.
- A Timer_A interval (`TA0`, ACLK, `TA0CCR0 = 20000`, approximately 0.6 s at
  32.768 kHz) wakes the CPU from LPM3.
- Each wake-up performs one conversion and transmits four bytes:

  ```
  [ ADC_high, ADC_low, 0x01, 0x02 ]
  ```

- RF output power is set with `WriteSinglePATable(0xc3)`.
- Radio register settings are in `HAL/RF1A.c` (`WriteRfSettings()`).

### Receiver node (`BWTS-Receiver-demo`)

- Uses the same radio settings as the sensor node.
- Accepts a packet only when the trailer is `0x01 0x02`.
- Converts the 12-bit result to a voltage:

  ```
  V = ADC * 1.5 / 4096 - 0.023
  ```

- Sends the **raw ADC value** as a decimal ASCII string followed by `\n` over
  UART on **P1.5/P1.6**, **9600 8N1**, clocked from ACLK.
- Toggles the on-board LEDs for every valid packet.

## Repository layout

```
.
├── BWST-sensor-demo/          # CCS project: sensor node
│   ├── BWST-sensor-demo.c     # application entry point
│   ├── BWST-sensor-demo.h     # project header (replaces TI's RF_Toggle_LED_Demo.h)
│   ├── HAL/                   # RF1A radio driver (TI, BSD)
│   ├── targetConfigs/         # CCS target configuration (.ccxml)
│   └── ...
├── BWTS-Receiver-demo/        # CCS project: receiver node
│   ├── BWTS-Reciver-demo.c    # application entry point (original filename)
│   ├── BWTS-Receiver-demo.h   # project header (replaces TI's RF_Toggle_LED_Demo.h)
│   ├── HAL/                   # RF1A, port-mapper and PMM drivers (TI, BSD)
│   ├── targetConfigs/
│   └── ...
├── THIRD_PARTY_NOTICES.md
├── CITATION.cff
└── LICENSE
```

## Hardware

- TI CC430F5137 (or a pin-compatible CC430 variant)
- Strain-gauge bridge connected to A0; bridge excitation on P2.2
- Sub-1 GHz antenna matched to the target band
- UART header on P1.5/P1.6
- TI MSP-FET / eZ430 debug probe

The energy-harvesting front end is external to this firmware and is described
in the accompanying paper.

## Building with Code Composer Studio

1. Install **Code Composer Studio** with MSP430 support. The projects were
   created with the CCS 8-era tool chain (codegen 3.2.3).
2. `File → Import → CCS Projects`, then select both project folders.
3. Select the **CC430F5137** target. The exported `.ccsproject` metadata may
   mention CC430F6137 — both variants share the same CPU and radio; the
   `targetConfigs/` folder contains `CC430F5137.ccxml` (and `CC430F6137.ccxml`
   in the receiver project).
4. Build and flash with the MSP-FET.

No external files need to be downloaded: all third-party code shipped in this
repository is BSD-licensed.

## TI example code and replacements

TI's CC430 RF examples package (SLAC525) is distributed under the restrictive
*MSP Source and Object Code Software License Agreement* and is not
redistributed here. The code that depended on it has been adapted as follows:

| Original TI file | Replacement |
|---|---|
| `RF_Toggle_LED_Demo.h` (sensor) | `BWST-sensor-demo.h` (project-local, MIT) |
| `RF_Toggle_LED_Demo.h` (receiver) | `BWTS-Receiver-demo.h` (project-local, MIT) |
| `HAL_PMM.c/.h` (receiver) | BSD-licensed `HAL/HAL_PMM.c/.h` from TI's F5xx/6xx core library |
| `HAL_MACROS.h` (receiver) | BSD-licensed `HAL/HAL_MACROS.h` |
| `HAL_MACROS.h` / `HAL_PMM.*` (sensor) | not needed — the sensor does not use the PMM |

The original `RF_Toggle_LED_Demo.h` contained only includes and function
declarations, so the project-local headers are functionally equivalent.

> The BSD `HAL_PMM` implementation is a different revision from the one in
> SLAC525. Verify `SetVCore(3)` on hardware before relying on it; if needed, a
> minimal `SetVCore` can be implemented from TI's public MSP430 User's Guide.

## UART output

```
<raw 12-bit ADC value>\n
```

Example:

```
1834
```

The voltage corresponding to a sample is
`V = ADC * 1.5 / 4096 - 0.023` (volts), as computed on the receiver.

## Third-party code and licenses

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The MIT license in
[LICENSE](LICENSE) covers only the original code authored by Pengyu Li.
TI and other third-party files remain under their own licenses, reproduced in
their file headers.

## Citation

If you use this firmware, please cite the accompanying paper. See
[CITATION.cff](CITATION.cff) and update the publication details once the paper
is published.

## Contact

**Pengyu Li** — STVLAb, City University of Hong Kong

## Disclaimer

This firmware is provided for research and educational purposes, "as is",
without warranty of any kind. It has not been certified for safety-critical or
medical applications.
