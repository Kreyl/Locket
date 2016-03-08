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

#define APP_NAME        "Radomir01"
#define APP_VERSION     _TIMENOW_


// ==== Constants and default values ====
#define ID_MIN                  1
#define ID_MAX                  10
#define ID_DEFAULT              ID_MIN

// Timings
#define CHECK_PERIOD_MS         3600
#define INDICATION_PERIOD_MS    1800
#define MINUTES_15_S            (15 * 60)
#define MINUTES_45_S            (45 * 60)

#if 1 // ==== Eeprom ====
// Addresses
#define EE_ADDR_DEVICE_ID       0
#define EE_ADDR_COLOR           4
#endif

enum Mode_t {
    mRx = 0b0000,
    mRxTxLo    = 0b0001, mRxTxHi    = 0b1001,
    mRxTxOffLo = 0b0010, mRxTxOffHi = 0b1010,
    mTxLo      = 0b0011, mTxHi      = 0b1011,
    mTx1515Lo  = 0b0100, mTx1515Hi  = 0b1100,
    mBadMode = 0b1111
};

enum TxState_t { tosOn, tosOff, tosOnAndSwitchOffDisabled };

class App_t {
private:
    Thread *PThread;
    Eeprom_t EE;
    uint8_t ISetID(int32_t NewID);
    const BaseChunk_t *VibroSequence;
public:
    int32_t ID;
    Mode_t Mode = mBadMode;
    TxState_t TxState = tosOff;
    VirtualTimer TmrCheck, Tmr15, Tmr45, TmrIndication;
    void CheckRxTable();
    uint8_t GetDipSwitch();
    void ReadIDfromEE();
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
};

extern App_t App;


#endif /* MAIN_H_ */
