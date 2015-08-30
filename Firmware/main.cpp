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

//Beeper_t Beeper;
Vibro_t Vibro(GPIOB, 8, TIM4, 3);
LedRGB_t Led { {GPIOB, 1, TIM3, 4}, {GPIOB, 0, TIM3, 3}, {GPIOB, 5, TIM3, 2} };

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
    Clk.UpdateFreqValues();
    // ==== Init OS ====
    halInit();
    chSysInit();
    // ==== Init Hard & Soft ====
    Uart.Init(115200);
    App.InitThread();
    App.ReadIDfromEE();

    Led.Init();
#if BTN_ENABLED
    PinSensors.Init();
#endif
    Uart.Printf("\r%S v%S ID=%u", APP_NAME, APP_VERSION, App.ID);

    Vibro.Init();
    Vibro.StartSequence(vsqBrr);
    // Init random generator with ID
    srand(App.ID);
    // Disable radio
    if(CC.Init() == OK) CC.EnterPwrDown();

    App.ITask();
}

__attribute__ ((__noreturn__))
void App_t::ITask() {
    // Timer vibro
    TmrVibro.InitAndStart(chThdSelf(), S2ST(VIBRO_PERIOD_S), EVTMSK_VIBRO, tvtPeriodic);
    SignalEvt(EVTMSK_VIBRO);    // Start vibro for test
    // Timer color
    ProcessTmrClr();

    // Main cycle
    while(true) {
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
#if 1   // ==== Uart cmd ====
        if(EvtMsk & EVTMSK_UART_NEW_CMD) {
            OnUartCmd(&Uart);
            Uart.SignalCmdProcessed();
        }
#endif

        if(EvtMsk & EVTMSK_CLR_SWITCH) ProcessTmrClr();
        if(EvtMsk & EVTMSK_VIBRO) Vibro.StartSequence(vsqBrrBrrNonStop);
        if(EvtMsk & EVTMSK_BTN) Vibro.Stop();

#if 0   // ==== Once a second ====
        if(EvtMsk & EVTMSK_SECOND) {
            DipToTxPwr();               // Check DIP switch
        }
#endif

#if 0 // ==== OFF ====
        if(EvtMsk & EVTMSK_OFF) {
            Radio.StopTx();
            chThdSleepMilliseconds(180);
            // Enter Standby
            chSysLock();
            Sleep::EnableWakeup1Pin();
            Sleep::EnterStandby();
            chSysUnlock();
        }
#endif
    } // while true
}

void App_t::ProcessTmrClr() {
    // Switch color
    if(IsGreen) {
        IsGreen = false;
        Led.StartSequence(lsqRed);
    }
    else {
        IsGreen = true;
        Led.StartSequence(lsqGreen);
    }
    // Calculate random time: [CLR_SW_TIME_MIN_S; CLR_SW_TIME_MAX_S]
    uint32_t Delay = (rand() % ((CLR_SW_TIME_MAX_S + 1) - CLR_SW_TIME_MIN_S)) + CLR_SW_TIME_MIN_S;
    Uart.Printf("\rDelay=%u", Delay);
    TmrColor.InitAndStart(PThread, S2ST(Delay), EVTMSK_CLR_SWITCH, tvtOneShot);
}

void ProcessButton(PinSnsState_t *PState, uint32_t Len) {
    if(*PState == pssRising) App.SignalEvt(EVTMSK_BTN);
}

void App_t::OnUartCmd(Uart_t *PUart) {
    UartCmd_t *PCmd = &PUart->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("\r%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) PUart->Ack(OK);

    else if(PCmd->NameIs("GetID")) Uart.Reply("ID", ID);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNextNumber(&dw32) != OK) { PUart->Ack(CMD_ERROR); return; }
        uint8_t r = ISetID(dw32);
        PUart->Ack(r);
    }

    else PUart->Ack(CMD_UNKNOWN);
}

void App_t::ReadIDfromEE() {
    ID = EE.Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(ID < ID_MIN or ID > ID_MAX) {
        Uart.Printf("\rUsing default ID");
        ID = ID_DEFAULT;
    }
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
}

uint8_t App_t::GetDipSwitch() {
    PinSetupIn(DIPSWITCH_GPIO, DIPSWITCH_PIN1, pudPullUp);
    PinSetupIn(DIPSWITCH_GPIO, DIPSWITCH_PIN2, pudPullUp);
    PinSetupIn(DIPSWITCH_GPIO, DIPSWITCH_PIN3, pudPullUp);
    PinSetupIn(DIPSWITCH_GPIO, DIPSWITCH_PIN4, pudPullUp);
    uint8_t Rslt = 0;
    if(PinIsSet(DIPSWITCH_GPIO, DIPSWITCH_PIN1)) Rslt = 2; // <=> { Rslt=1; Rslt<<=1; }
    if(PinIsSet(DIPSWITCH_GPIO, DIPSWITCH_PIN2)) Rslt |= 1;
    Rslt <<= 1;
    if(PinIsSet(DIPSWITCH_GPIO, DIPSWITCH_PIN3)) Rslt |= 1;
    Rslt <<= 1;
    if(PinIsSet(DIPSWITCH_GPIO, DIPSWITCH_PIN4)) Rslt |= 1;
    Rslt ^= 0x0F;    // Invert switches
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN1);
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN2);
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN3);
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN4);
    return Rslt;
}
