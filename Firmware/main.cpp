/*
 * File:   main.cpp
 * Author: Kreyl
 * Project: Salem Box
 *
 * Created on Mar 22, 2015, 01:23
 */

#include <buttons.h>
#include "main.h"
#include "SimpleSensors.h"
#include "Sequences.h"
#include "led.h"
#include "vibro.h"
#include "radio_lvl1.h"

App_t App;
// Dip Switch
#define DIPSWITCH_GPIO  GPIOA
#define DIPSWITCH_PIN1  8
#define DIPSWITCH_PIN2  11
#define DIPSWITCH_PIN3  12
#define DIPSWITCH_PIN4  15

Vibro_t Vibro(GPIOB, 8, TIM4, 3);

static const uint8_t PwrTable[12] = {
		CC_PwrMinus30dBm, // 0
		CC_PwrMinus27dBm, // 1
		CC_PwrMinus25dBm, // 2
		CC_PwrMinus20dBm, // 3
		CC_PwrMinus15dBm, // 4
		CC_PwrMinus10dBm, // 5
		CC_PwrMinus6dBm,  // 6
		CC_Pwr0dBm,       // 7
		CC_PwrPlus5dBm,   // 8
		CC_PwrPlus7dBm,   // 9
		CC_PwrPlus10dBm,  // 10
		CC_PwrPlus12dBm   // 11
};

#if 1 // ============================ Timers ===================================
// Once-a-second timer
void TmrSecondCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI(EVTMSK_EVERY_SECOND);
    chVTSetI(&App.TmrSecond, MS2ST(1000), TmrSecondCallback, nullptr);
    chSysUnlockFromIsr();
}
// Universal VirtualTimer callback
void TmrGeneralCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI((eventmask_t)p);
    chSysUnlockFromIsr();
}
#endif

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
    Clk.UpdateFreqValues();

    // ==== Init OS ====
    halInit();
    chSysInit();

    // ==== Init Hard & Soft ====
    App.InitThread();
    App.ReadIDfromEE();

    // Timers
    chVTSet(&App.TmrSecond, MS2ST(1000), TmrSecondCallback, nullptr);

    Uart.Init(115200);
    Uart.Printf("\r%S_%S  ID=%d", APP_NAME, APP_VERSION, App.ID);

    Vibro.Init();

    if(Radio.Init() != OK) {
    	Vibro.StartSequence(vsqError);
        chThdSleepMilliseconds(2700);
    }
    else Vibro.StartSequence(vsqBrrBrr);

    // Main cycle
    App.ITask();
}

__attribute__ ((__noreturn__))
void App_t::ITask() {
    while(true) {
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
#if 1   // ==== Uart cmd ====
        if(EvtMsk & EVTMSK_UART_NEW_CMD) {
            OnUartCmd(&Uart);
            Uart.SignalCmdProcessed();
        }
#endif

#if 1   // ==== Every second ====
        if(EvtMsk & EVTMSK_EVERY_SECOND) {
            // Get power
            uint8_t b = GetDipSwitch();
            Radio.Pwr = (b > 11)? CC_PwrPlus12dBm : PwrTable[b];
//            Uart.Printf("\rPwrID=%X", Radio.Pwr);
        } // if EVTMSK_EVERY_SECOND
#endif
    } // while true
}

void App_t::OnUartCmd(Uart_t *PUart) {
    UartCmd_t *PCmd = &PUart->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("\r%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) PUart->Ack(OK);

    else if(PCmd->NameIs("GetID")) Uart.Reply("ID", ID);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNextToken() != OK) { PUart->Ack(CMD_ERROR); return; }
        if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { PUart->Ack(CMD_ERROR); return; }
        uint8_t r = ISetID(dw32);
        PUart->Ack(r);
    }

    else PUart->Ack(CMD_UNKNOWN);
}

uint8_t App_t::GetDipSwitch() {
    PinSetupIn(DIPSWITCH_GPIO, DIPSWITCH_PIN1, pudPullUp);
    PinSetupIn(DIPSWITCH_GPIO, DIPSWITCH_PIN2, pudPullUp);
    PinSetupIn(DIPSWITCH_GPIO, DIPSWITCH_PIN3, pudPullUp);
    PinSetupIn(DIPSWITCH_GPIO, DIPSWITCH_PIN4, pudPullUp);
    uint8_t Rslt;
    Rslt  = static_cast<uint8_t>(PinIsSet(DIPSWITCH_GPIO, DIPSWITCH_PIN1));
    Rslt <<= 1;
    Rslt |= static_cast<uint8_t>(PinIsSet(DIPSWITCH_GPIO, DIPSWITCH_PIN2));
    Rslt <<= 1;
    Rslt |= static_cast<uint8_t>(PinIsSet(DIPSWITCH_GPIO, DIPSWITCH_PIN3));
    Rslt <<= 1;
    Rslt |= static_cast<uint8_t>(PinIsSet(DIPSWITCH_GPIO, DIPSWITCH_PIN4));
    Rslt ^= 0xF;    // Invert switches
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN1);
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN2);
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN3);
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN4);
    return Rslt;
}

void App_t::ReadIDfromEE() {
    ID = EE.Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(ID < ID_MIN or ID > ID_MAX) ID = ID_DEFAULT;
}

uint8_t App_t::ISetID(int32_t NewID) {
    if(NewID < ID_MIN or NewID > ID_MAX) return FAILURE;
    uint8_t rslt = EE.Write32(EE_ADDR_DEVICE_ID, NewID);
    if(rslt == OK) {
        ID = NewID;
        Uart.Printf("\rNew ID: %u", ID);
        return OK;
    }
    else {
        Uart.Printf("\rEE error: %u", rslt);
        return FAILURE;
    }
    return FAILURE;
}
