/*
\ * main.h
 *
 *  Created on: 21 дек. 2014 г.
 *      Author: Kreyl
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "kl_lib.h"
#include "ch.h"
#include "hal.h"
#include "evt_mask.h"
#include "uart.h"
#include "color.h"
#include "ChunkTypes.h"

#define APP_VERSION     _TIMENOW_

// ==== Constants and default values ====
#define ID_MIN                  41
#define ID_MAX                  50
#define ID_DEFAULT              ID_MIN

enum Mode_t {modeNone, modeDetectorTx, modeDetectorRx};

class App_t {
private:
    thread_t *PThread;
public:
    int32_t ID;
    Mode_t Mode = modeNone;
    // Eternal methods
    void InitThread() { PThread = chThdGetSelfX(); }
    void SignalEvt(eventmask_t Evt) {
        chSysLock();
        chEvtSignalI(PThread, Evt);
        chSysUnlock();
    }
    void SignalEvtI(eventmask_t Evt) { chEvtSignalI(PThread, Evt); }
    void OnCmd(Shell_t *PShell);
    // Inner use
    void ITask();
};

extern App_t App;


#endif /* MAIN_H_ */
