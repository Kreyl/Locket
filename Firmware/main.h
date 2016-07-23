/*
\ * main.h
 *
 *  Created on: 21 дек. 2014 г.
 *      Author: Kreyl
 */

#pragma once

#include "kl_lib.h"
#include "ch.h"
#include "hal.h"
#include "evt_mask.h"
#include "uart.h"
#include "color.h"

#define APP_NAME        "BtnColorChangeTx"
#define APP_VERSION     _TIMENOW_

// ==== Constants and default values ====
#define ID_MIN                  1
#define ID_MAX                  25
#define ID_DEFAULT              ID_MIN

// Timings

#define BTN_ENABLED             TRUE

// Dip Switch
#define DIPSWITCH_GPIO          GPIOA
#define DIPSWITCH_PIN1          8
#define DIPSWITCH_PIN2          11
#define DIPSWITCH_PIN3          12
#define DIPSWITCH_PIN4          15

#if 1 // ==== Eeprom ====
// Addresses
#define EE_ADDR_DEVICE_ID       0
#define EE_ADDR_COLOR           4
#endif

extern int32_t appID;
extern Color_t txColor;

void appSignalEvt(eventmask_t Evt);
void appSignalEvtI(eventmask_t Evt);
