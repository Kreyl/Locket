/*
 * main.h
 *
 *  Created on: 21 дек. 2014 г.
 *      Author: Kreyl
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "kl_lib_L15x.h"
#include "ch.h"
#include "hal.h"
#include "clocking_L1xx.h"
#include "evt_mask.h"
#include "uart.h"

#define VERSION_STRING  "SalemTX v1.0"

// ==== Constants and default values ====
#define ID_MIN                  1
#define ID_MAX                  15
#define ID_DEFAULT              ID_MIN

class App_t {
private:
    Thread *PThread;
public:
    uint32_t ID;
    uint8_t GetDipSwitch();
    // Eternal methods
    void InitThread() { PThread = chThdSelf(); }
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


#endif /* MAIN_H_ */
