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

#define APP_NAME        "BtnTxAllRx"
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

class App_t {
private:
    Thread *PThread;
    Eeprom_t EE;
    uint8_t ISetID(int32_t NewID);
public:
    int32_t ID;
    bool MustTransmit = false;
    TmrVirtual_t TmrTxOff;
    // Eternal methods
    void ReadIDfromEE();
    uint8_t GetDipSwitch();
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
//    App_t(): PThread(nullptr), ID(ID_DEFAULT), Mode(mRxVibro) {}
};

extern App_t App;


#endif /* MAIN_H_ */
