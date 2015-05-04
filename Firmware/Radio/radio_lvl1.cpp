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
//extern LedRGB_t Led;

__attribute__((__noreturn__))
void rLevel1_t::ITask() {
    while(true) {
#ifdef TX
        CC.SetChannel(ID2RCHNL(App.ID));
        Pkt.ID = App.ID;
        Pkt.TestWord = TEST_WORD;
        // Transmit
        DBG1_SET();
        CC.TransmitSync(&Pkt);
        DBG1_CLR();
        //chThdSleepMilliseconds(99);
#elif defined LED_RX
        Color_t Clr;
        int8_t Rssi;
//        if(Enabled) {
            uint8_t RxRslt = CC.ReceiveSync(306, &Pkt, &Rssi);
            if(RxRslt == OK) {
                Uart.Printf("%d\r", Rssi);
                Clr = clWhite;
                if     (Rssi < -100) Clr = clRed;
                else if(Rssi < -90) Clr = clYellow;
                else if(Rssi < -80) Clr = clGreen;
                else if(Rssi < -70) Clr = clCyan;
                else if(Rssi < -60) Clr = clBlue;
                else if(Rssi < -50) Clr = clMagenta;
            }
            else {
                Clr = clBlack;
    //            Uart.Printf("Halt\r");
            }
            Led.SetColor(Clr);
//        }
        chThdSleepMilliseconds(99);
#endif
    } // while true
}
#endif // task

#if 1 // ============================
uint8_t rLevel1_t::Init() {
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
        return OK;
    }
    else {
        Uart.Printf("\rCC init error");
        return FAILURE;
    }
}
#endif
