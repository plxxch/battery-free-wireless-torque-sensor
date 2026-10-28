/*
 * BWST-sensor-demo.h
 *
 * Project header for the BWST sensor node.
 *
 * This replaces the TI example header RF_Toggle_LED_Demo.h. That file
 * contained only includes and function declarations (no defines or function
 * bodies), so an equivalent project-local header is used instead.
 *
 * Copyright (c) 2023 Pengyu Li
 * SPDX-License-Identifier: MIT
 */

#ifndef BWST_SENSOR_DEMO_H
#define BWST_SENSOR_DEMO_H

#include "cc430x513x.h"
#include "RF1A.h"

void Transmit(unsigned char *buffer, unsigned char length);
void ReceiveOn(void);
void ReceiveOff(void);
void InitRadio(void);

#endif /* BWST_SENSOR_DEMO_H */
