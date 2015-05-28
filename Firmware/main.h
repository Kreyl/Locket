/*
 * main.h
 *
 *  Created on: 21 дек. 2014 г.
 *      Author: Kreyl
 */

#ifndef MAIN_H_
#define MAIN_H_

#if defined STM32F0XX
#include "kl_lib_f0xx.h"
#else
#include "kl_lib_L15x.h"
#endif

#include "ch.h"
#include "hal.h"
#include <clocking_f030.h>
#include "evt_mask.h"
#include "uart.h"

#define VERSION_STRING  "Common v1.0"

// ==== Constants and default values ====
#define ID_MIN                  1
#define ID_MAX                  15
#define ID_DEFAULT              ID_MIN

class App_t {
private:
    thread_t *PThread;
public:
    uint32_t ID;
    uint8_t GetDipSwitch();
    // Eternal methods
    void InitThread() { PThread = chThdGetSelfX(); }
    void SignalEvt(eventmask_t Evt) {
        chSysLock();
        chEvtSignalI(PThread, Evt);
        chSysUnlock();
    }
    void SignalEvtI(eventmask_t Evt) { chEvtSignalI(PThread, Evt); }
    // Inner use
    void ITask();
    App_t(): PThread(nullptr), ID(ID_DEFAULT) {}
};

extern App_t App;

// MCU-dependent constants
#ifdef STM32F030x8
#define TIM_VIBRO   TIM16
#endif

// MCU-independent constants
// Dip Switch
#define DIPSWITCH_GPIO  GPIOA
#define DIPSWITCH_PIN1  8
#define DIPSWITCH_PIN2  11
#define DIPSWITCH_PIN3  12
#define DIPSWITCH_PIN4  15



#endif /* MAIN_H_ */
