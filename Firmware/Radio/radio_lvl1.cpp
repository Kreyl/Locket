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
        // ==== RX ====
        if(App.Mode >= mRxVibro and App.Mode <= mRxVibroLight) {
            int8_t Rssi;
            // Iterate channels
            for(int32_t i = ID_MIN; i <= ID_MAX; i++) {
                if(i == App.ID) continue;   // Do not listen self
                CC.SetChannel(ID2RCHNL(i));
                uint8_t RxRslt = CC.ReceiveSync(RX_T_MS, &Pkt, &Rssi);
                if(RxRslt == OK) {
//                    Uart.Printf("\rCh=%d; Rssi=%d", i, Rssi);
                    if(Pkt.DWord == TEST_WORD) App.SignalEvt(EVTMSK_RADIO_RX);
                    break;  // No need to listen anymore
                }
            } // for
            CC.EnterPwrDown();
            chThdSleepMilliseconds(RX_SLEEP_T_MS);
        } // if rx

        // ==== TX ====
        else if(App.Mode >= mTxLowPwr and App.Mode <= mTxMaxPwr) {
            CC.SetChannel(ID2RCHNL(App.ID));
            Pkt.DWord = TEST_WORD;
            SetTxPwr();
            DBG1_SET();
            CC.TransmitSync(&Pkt);
            DBG1_CLR();
            chThdSleepMilliseconds(TX_PERIOD_MS);
        } // if tx

        // ==== RxTx: see each other ====
        else if(App.Mode >= mRxTxVibroLow and App.Mode <= mRxTxLightMax) {
            CC.SetChannel(RCHNL_RXTX);
            // Iterate cycles
            for(uint32_t CycleN=0; CycleN < CYCLE_CNT; CycleN++) {
                // ==== New cycle begins ====
                uint32_t TxSlot = rand() % SLOT_CNT;    // Choose slot to transmit
        //        Uart.Printf("\rTxSlot: %u", TxSlot);
                // If TX slot is not zero: receive at zero cycle or sleep otherwise
                uint32_t Delay = TxSlot * SLOT_DURATION_MS;
                if(Delay != 0) {
                    if(CycleN == 0) TryToReceive(Delay);
                    else TryToSleep(Delay);
                }
                // Transmit
                Pkt.DWord = App.ID;
                SetTxPwr();
                DBG1_SET();
                CC.TransmitSync(&Pkt);
                DBG1_CLR();
                // If TX slot is not last, receive at zero cycle or sleep otherwise
                Delay = (SLOT_CNT - TxSlot - 1) * SLOT_DURATION_MS;
                if(Delay != 0) {
                    if(CycleN == 0) TryToReceive(Delay);
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

void rLevel1_t::TryToReceive(uint32_t RxDuration) {
    int8_t Rssi;
    uint32_t TimeEnd = chTimeNow() + RxDuration;
//    Uart.Printf("\r***End: %u", TimeEnd);
    while(true) {
        DBG2_SET();
        uint8_t RxRslt = CC.ReceiveSync(RxDuration, &Pkt, &Rssi);
        DBG2_CLR();
        if(RxRslt == OK) {
//            Uart.Printf("\rRID = %X", PktRx.UID);
            IdBuf.Put(Pkt.DWord);
        }
//        Uart.Printf("\rNow: %u", chTimeNow());
        if(chTimeNow() < TimeEnd) RxDuration = TimeEnd - chTimeNow();
        else break;
    }
//    Uart.Printf("\rDif: %u", chTimeNow() - TimeEnd);
}

void rLevel1_t::TryToSleep(uint32_t SleepDuration) {
    if(SleepDuration < MIN_SLEEP_DURATION_MS) return;
    else {
        CC.EnterPwrDown();
        chThdSleepMilliseconds(SleepDuration);
    }
}

void rLevel1_t::SetTxPwr() {
    switch(App.Mode) {
        case mTxLowPwr:
        case mRxTxVibroLow:
        case mRxTxLightLow:
            CC.SetTxPower(CC_PwrMinus10dBm);
            break;
        case mTxMidPwr:
        case mRxTxVibroMid:
        case mRxTxLightMid:
            CC.SetTxPower(CC_Pwr0dBm);
            break;
        case mTxHiPwr:
        case mRxTxVibroHi:
        case mRxTxLightHi:
            CC.SetTxPower(CC_PwrPlus5dBm);
            break;
        case mTxMaxPwr:
        case mRxTxVibroMax:
        case mRxTxLightMax:
            CC.SetTxPower(CC_PwrPlus10dBm);
            break;
        default: break;
    }
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
