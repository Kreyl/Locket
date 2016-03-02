/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "evt_mask.h"
#include "main.h"
#include "cc1101.h"
#include "uart.h"
#include "led.h"

#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOB
#define DBG_PIN1    14
#define DBG1_SET()  PinSet(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinClear(DBG_GPIO1, DBG_PIN1)
#define DBG_GPIO2   GPIOB
#define DBG_PIN2    15
#define DBG2_SET()  PinSet(DBG_GPIO2, DBG_PIN2)
#define DBG2_CLR()  PinClear(DBG_GPIO2, DBG_PIN2)
#endif

rLevel1_t Radio;

#if 1 // ================================ Task =================================
static WORKING_AREA(warLvl1Thread, 256);
__attribute__((__noreturn__))
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    Radio.ITask();
}

__attribute__((__noreturn__))
void rLevel1_t::ITask() {
    while(true) {
#if 0        // Demo
        if(App.Mode == 0b0001) { // RX
            int8_t Rssi;
            Color_t Clr;
            uint8_t RxRslt = CC.ReceiveSync(RX_T_MS, &Pkt, &Rssi);
            if(RxRslt == OK) {
                Uart.Printf("\rRssi=%d", Rssi);
                Clr = clWhite;
                if     (Rssi < -100) Clr = clRed;
                else if(Rssi < -90) Clr = clYellow;
                else if(Rssi < -80) Clr = clGreen;
                else if(Rssi < -70) Clr = clCyan;
                else if(Rssi < -60) Clr = clBlue;
                else if(Rssi < -50) Clr = clMagenta;
            }
            else Clr = clBlack;
            Led.SetColor(Clr);
            chThdSleepMilliseconds(99);
        }
        else {  // TX
            DBG1_SET();
            CC.TransmitSync(&Pkt);
            DBG1_CLR();
//            chThdSleepMilliseconds(99);
        }
#else
        if(App.Mode != mBadMode) {
            CC.SetChannel(RCHNL_RXTX);
            // Iterate cycles
            for(uint32_t CycleN=0; CycleN < CYCLE_CNT; CycleN++) {
                // ==== New cycle begins ====
                uint32_t TxSlot = rand() % SLOT_CNT;    // Choose slot to transmit
                // If TX slot is not zero: receive at zero cycle or sleep otherwise
                uint32_t Delay = TxSlot * SLOT_DURATION_MS;
                if(Delay != 0) {
                    if(CycleN == 0 and RxTable.GetCount() < RXTABLE_MAX_CNT) Receive(Delay);
                    else TryToSleep(Delay);
                }
                // Transmit if allowed
                if(App.TxState == tosOff) chThdSleepMilliseconds(SLOT_DURATION_MS);
                else Transmit();
                // If TX slot is not last, receive at zero cycle or sleep otherwise
                Delay = (SLOT_CNT - TxSlot - 1) * SLOT_DURATION_MS;
                if(Delay != 0) {
                    if(CycleN == 0 and RxTable.GetCount() < RXTABLE_MAX_CNT) Receive(Delay);
                    else TryToSleep(Delay);
                }
            } // for CycleN
        } // If RxTx

        // Errorneous mode
        else chThdSleepMilliseconds(450);
#endif
    } // while true
}
#endif // task

void rLevel1_t::Receive(uint32_t RxDuration) {
    int8_t Rssi;
    uint32_t TimeEnd = chTimeNow() + RxDuration;
    while(true) {
        DBG2_SET();
        uint8_t RxRslt = CC.ReceiveSync(RxDuration, &Pkt, &Rssi);
        DBG2_CLR();
        if(RxRslt == OK) {
            Uart.Printf("\r***RID = %X", Pkt.DWord);
            chSysLock();
            RxTable.Add(Pkt.DWord);
            chSysUnlock();
        }
        if(chTimeNow() < TimeEnd) RxDuration = TimeEnd - chTimeNow();
        else break;
    }
}

void rLevel1_t::TryToSleep(uint32_t SleepDuration) {
    if(SleepDuration >= MIN_SLEEP_DURATION_MS) CC.EnterPwrDown();
    chThdSleepMilliseconds(SleepDuration);
}

void rLevel1_t::Transmit() {
    // Set TX power
    switch(App.Mode) {
        case mRxTxLo:
        case mRxTxOffLo:
        case mTxLo:
        case mTx1515Lo:
            CC.SetTxPower(CC_PwrMinus20dBm);
            break;
        case mRxTxHi:
        case mRxTxOffHi:
        case mTxHi:
        case mTx1515Hi:
            CC.SetTxPower(CC_PwrMinus10dBm);
            break;
        default: break;
    }
    Pkt.DWord = App.ID;
    DBG1_SET();
    CC.TransmitSync(&Pkt);
    DBG1_CLR();
}

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif    // Init radioIC
    if(CC.Init() == OK) {
        CC.SetTxPower(CC_Pwr0dBm);
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(0);
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return OK;
    }
    else return FAILURE;
}
#endif
