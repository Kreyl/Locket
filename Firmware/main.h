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
#define EE_ADDR_DEVICE_ID       0
#endif

enum Mode_t {
    mError = 0b0000,
    mRxVibro  = 0b0001, mRxLight  = 0b0010, mRxVibroLight = 0b0011,
    mTxLowPwr = 0b0100, mTxMidPwr = 0b0101, mTxHiPwr = 0b0110, mTxMaxPwr = 0b0111,
    mRxTxVibroLow = 0b1000, mRxTxVibroMid = 0b1001, mRxTxVibroHi = 0b1010, mRxTxVibroMax = 0b1011,
    mRxTxLightLow = 0b1100, mRxTxLightMid = 0b1101, mRxTxLightHi = 0b1110, mRxTxLightMax = 0b1111
};

enum IndicationState_t {isOn, isOff};

class App_t {
private:
    Thread *PThread;
    Eeprom_t EE;
    VirtualTimer ITmrRadioTimeout;
    IndicationState_t Indication;
    uint8_t ISetID(int32_t NewID);
public:
    int32_t ID;
    uint8_t GetDipSwitch();
    Mode_t Mode;
    VirtualTimer TmrSecond;
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
    App_t(): PThread(nullptr), Indication(isOff), ID(ID_DEFAULT), Mode(mError) {}
};

extern App_t App;


#endif /* MAIN_H_ */
