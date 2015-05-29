/*
 * main.h
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

#define VERSION_STRING  "Klaus v1.0"

// ==== Constants and default values ====
#define ID_MIN                  1
#define ID_MAX                  10
#define ID_DEFAULT              ID_MIN
// Radio timing
#define RADIO_NOPKT_TIMEOUT_S   4


#if 1 // ==== Eeprom ====
// Addresses
#define EE_DEVICE_ID_ADDR       0
#endif

enum Mode_t {
    mError = 0b000,
    mRxVibro = 0b001, mRxLight = 0b010, mRxVibroLight = 0b011,
    mTxLowPwr = 0b100, mTxMidPwr = 0b101, mTxHiPwr = 0b110, mTxMaxPwr = 0b111
};

class App_t {
private:
    Thread *PThread;
    VirtualTimer ITmrRadioTimeout;
    uint8_t ISetID(int32_t NewID);
public:
    int32_t ID;
    uint8_t GetDipSwitch();
    Mode_t Mode;
    VirtualTimer TmrSecond;
    // Eternal methods
    void InitThread() { PThread = chThdSelf(); }
    void SignalEvt(eventmask_t Evt) {
        chSysLock();
        chEvtSignalI(PThread, Evt);
        chSysUnlock();
    }
    void SignalEvtI(eventmask_t Evt) { chEvtSignalI(PThread, Evt); }
    void OnUartCmd(Uart_t *PUart);
    // Inner use
    void ITask();
    App_t(): PThread(nullptr), ID(ID_DEFAULT), Mode(mRxVibro) {}
};

extern App_t App;


#endif /* MAIN_H_ */
