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

//Beeper_t Beeper;
Vibro_t Vibro(GPIOB, 8, TIM4, 3);
LedRGB_t Led({GPIOB, 1, TIM3, 4}, {GPIOB, 0, TIM3, 3}, {GPIOB, 5, TIM3, 2});

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
// Check radio buffer
void TmrCheckCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI(EVTMSK_RX_BUF_CHECK);
    chVTSetI(&App.TmrCheck, MS2ST(RXBUF_CHECK_PERIOD_MS), TmrCheckCallback, nullptr);
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
    chVTSet(&App.TmrCheck, MS2ST(RXBUF_CHECK_PERIOD_MS), TmrCheckCallback, nullptr);

    Uart.Init(115200);
    Uart.Printf("\r%S_%S  ID=%d   AHB freq=%u", APP_NAME, APP_VERSION, App.ID, Clk.AHBFreqHz);

//    Beeper.Init();
//    Beeper.StartSequence(bsqBeepBeep);

    Led.Init();

    Vibro.Init();
    Vibro.StartSequence(vsqBrr);

    if(Radio.Init() != OK) {
        Led.StartSequence(lsqFailure);
        chThdSleepMilliseconds(2700);
    }

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
            // Get mode
            uint8_t b = GetDipSwitch();
            Mode_t NewMode = static_cast<Mode_t>(b);
            if(Mode != NewMode) {
                Mode = NewMode;
                Led.Stop();
                Vibro.Stop();
                LightWasOn = false;
                Uart.Printf("\rMode=%u", Mode);
                // Indicate Mode
                Led.StartSequence(lsqModesTable[static_cast<uint32_t>(Mode)]);
            }
            if(Mode == mError) Led.StartSequence(lsqFailure);
        }
#endif

#if 1   // ==== Rx buf check ====
        if(EvtMsk & EVTMSK_RX_BUF_CHECK) {
            // Get number of distinct received IDs and clear table
            chSysLock();
            uint32_t Cnt = Radio.RxTable.GetCount();
            Radio.RxTable.Clear();
            chSysUnlock();
            Uart.Printf("\rCnt = %u", Cnt);
            // ==== Select indication depending on received cnt ====
            const BaseChunk_t *VibroSequence = nullptr;
            bool LightMustBeOn = false;
            if(Cnt != 0) {
                switch(App.Mode) {
                    case mRxLight:
                    case mRxTxLightLow:
                    case mRxTxLightMid:
                    case mRxTxLightHi:
                    case mRxTxLightMax:
                        LightMustBeOn = true;
                        break;
                    case mRxVibro:
                        VibroSequence = vsqSingle;
                        break;
                    case mRxVibroLight:
                        LightMustBeOn = true;
                        VibroSequence = vsqSingle;
                        break;
                    case mRxTxVibroLow:
                    case mRxTxVibroMid:
                    case mRxTxVibroHi:
                    case mRxTxVibroMax:
                        if     (Cnt == 1) VibroSequence = vsqSingle;
                        else if(Cnt == 2) VibroSequence = vsqPair;
                        else              VibroSequence = vsqMany;  // RxTable.Cnt  > 2
                        break;
                    default: break;
                } // switch
            } // if != 0
            // ==== Indicate ====
            // Vibro
            if(VibroSequence == nullptr) Vibro.Stop();
            else if(Vibro.GetCurrentSequence() != VibroSequence) Vibro.StartSequence(VibroSequence);
            // Light
            if(LightMustBeOn and !LightWasOn) {
                LightWasOn = true;
                Led.StartSequence(lsqIndicationOn);
            }
            else if(!LightMustBeOn and LightWasOn) {
                LightWasOn = false;
                Led.StartSequence(lsqIndicationOff);
            }
        } // if evtmsk
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
