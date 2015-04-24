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

#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOC
#define DBG_PIN1    15
#define DBG1_SET()  PinSet(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinClear(DBG_GPIO1, DBG_PIN1)
#endif

rLevel1_t Radio;

#if 1 // ================================ Task =================================
static WORKING_AREA(warLvl1Thread, 256);
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    Radio.ITask();
}

__attribute__((__noreturn__)) void rLevel1_t::ITask() {
    while(true) {
#if 0 // ======== TX cycle ========
        switch(App.Type) {
            case dtLustraClean:
            case dtLustraWeak:
            case dtLustraStrong:
            case dtLustraLethal:
                // Setup channel, do nothing if bad ID
                if((App.ID >= LUSTRA_MIN_ID) and (App.ID <= LUSTRA_MAX_ID)) {
                    CC.SetChannel(LUSTRA_ID_TO_RCHNL(App.ID));
                    // Transmit corresponding pkt
                    uint8_t Indx = App.Type - dtLustraClean;
                    CC.TransmitSync((void*)&PktLustra[Indx]);
                }
                else {
                    Indication.LustraBadID();
                    chThdSleepMilliseconds(999);
                    continue;
                }
                break;

            case dtPelengator:
                CC.SetChannel(RCHNL_PELENG);
                for(uint8_t i=0; i<PELENG_TX_CNT; i++) CC.TransmitSync((void*)&PktDummy);
                break;

            case dtEmpGrenade:
                if(App.Grenade.State == gsRadiating) {
                    CC.SetChannel(RCHNL_EMP);
                    CC.TransmitSync((void*)&PktDummy);
                }
                else {
                    CC.Sleep();
                    chThdSleepMilliseconds(450);
                }
                break;

            default: break;
        } // switch
#endif

#if 1 // ============ RX cycle ============
        uint8_t RxRslt;
        CC.SetChannel(ID2RCHNL(App.Settings.ID));
        RxRslt = CC.ReceiveSync(RX_T_MS, &Pkt);
        if(RxRslt == OK) {
//            Uart.Printf("\rRx ID=%u; TestWord=%X", Pkt.ID, Pkt.TestWord);
            if(Pkt.TestWord == TEST_WORD and Pkt.ID == App.Settings.ID) {
                App.SignalEvt(EVTMSK_RADIO_RX);
            } // if test word
        } // if OK
        CC.EnterPwrDown();
        chThdSleepMilliseconds(RX_SLEEP_T_MS);
#endif // RX
    } // while true
}
#endif // task

#if 1 // ============================
void rLevel1_t::Init() {
    // Init radioIC
    if(CC.Init() == OK) {
        CC.SetTxPower(CC_Pwr0dBm);
        CC.SetPktSize(RPKT_LEN);
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
#ifdef DBG_PINS
        PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
#endif
//        Uart.Printf("\rCC init OK");
    }
    else Uart.Printf("\rCC init error");
}
#endif
