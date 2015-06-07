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
#define DBG_PIN1    15
#define DBG1_SET()  PinSet(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinClear(DBG_GPIO1, DBG_PIN1)
#endif

rLevel1_t Radio;

#if 1 // ================================ Task =================================
static WORKING_AREA(warLvl1Thread, 256);
__attribute__((__noreturn__))
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    Radio.ITask();
}

#define TX
#define LED_RX

__attribute__((__noreturn__))
void rLevel1_t::ITask() {
    while(true) {
        if(App.Mode == mError) chThdSleepMilliseconds(450);
        // ==== RX ====
        else if(App.Mode == mRxLight or App.Mode == mRxVibro or App.Mode == mRxVibroLight) {
            int8_t Rssi;
            // Iterate channels
            for(int32_t i = ID_MIN; i <= ID_MAX; i++) {
                if(i == App.ID) continue;   // Do not listen self
                CC.SetChannel(ID2RCHNL(i));
                uint8_t RxRslt = CC.ReceiveSync(RX_T_MS, &Pkt, &Rssi);
                if(RxRslt == OK) {
                    Uart.Printf("\rCh=%d; Rssi=%d", i, Rssi);
                    if(Pkt.TestWord == TEST_WORD) App.SignalEvt(EVTMSK_RADIO_RX);
                    break;  // No need to listen anymore
                }
            } // for
            CC.EnterPwrDown();
            chThdSleepMilliseconds(RX_SLEEP_T_MS);
        } // if rx

        // ==== TX ====
        else {
            CC.SetChannel(ID2RCHNL(App.ID));
            Pkt.TestWord = TEST_WORD;
            switch(App.Mode) {
                case mTxLowPwr: CC.SetTxPower(CC_PwrMinus10dBm); break;
                case mTxMidPwr: CC.SetTxPower(CC_Pwr0dBm);       break;
                case mTxHiPwr:  CC.SetTxPower(CC_PwrPlus5dBm);   break;
                case mTxMaxPwr: CC.SetTxPower(CC_PwrPlus12dBm);  break;
                default: break;
            }
            // Transmit
            DBG1_SET();
            CC.TransmitSync(&Pkt);
            DBG1_CLR();
            chThdSleepMilliseconds(TX_PERIOD_MS);
        } // if tx
    } // while true
}
#endif // task

#if 1 // ============================
uint8_t rLevel1_t::Init() {
    // Init radioIC
    if(CC.Init() == OK) {
//        CC.SetTxPower(CC_Pwr0dBm);
//        CC.SetPktSize(RPKT_LEN);
        // Thread
//        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
#ifdef DBG_PINS
        PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
#endif
//        Uart.Printf("\rCC init OK");
        return OK;
    }
    else {
        Uart.Printf("\rCC init error");
        return FAILURE;
    }
}
#endif
