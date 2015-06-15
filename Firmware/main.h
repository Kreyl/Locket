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

#define APP_NAME        "Klaus"
#define APP_VERSION     _TIMENOW_

// ==== Constants and default values ====
#define ID_MIN                  1
#define ID_MAX                  10
#define ID_DEFAULT              ID_MIN
// Colors
#define CLR_DEFAULT             clGreen
const Color_t ClrTable[] = {clRed, clGreen, clBlue, clYellow, clCyan, clMagenta, clWhite};
#define CLRTABLE_CNT    countof(ClrTable)

// Timings
#define INDICATION_PERIOD_MS    3600

#if 1 // ==== Eeprom ====
// Addresses
#define EE_ADDR_DEVICE_ID       0
#define EE_ADDR_COLOR           4
#endif

enum Mode_t {
    mColorSelect = 0b0000,
    mRxVibro  = 0b0001, mRxLight  = 0b0010, mRxVibroLight = 0b0011,
    mTxLowPwr = 0b0100, mTxMidPwr = 0b0101, mTxHiPwr = 0b0110, mTxMaxPwr = 0b0111,
    mRxTxVibroLow = 0b1000, mRxTxVibroMid = 0b1001, mRxTxVibroHi = 0b1010, mRxTxVibroMax = 0b1011,
    mRxTxLightLow = 0b1100, mRxTxLightMid = 0b1101, mRxTxLightHi = 0b1110, mRxTxLightMax = 0b1111
};

class App_t {
private:
    Thread *PThread;
    Eeprom_t EE;
    uint8_t ISetID(int32_t NewID);
    bool LightWasOn = false;
public:
    int32_t ID;
    Mode_t Mode;
    VirtualTimer TmrSecond, TmrIndication;
    void CheckRxTable();
    uint8_t GetDipSwitch();
    void ReadIDfromEE();
    void ReadColorFromEE();
    uint8_t SaveColorToEE();
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
