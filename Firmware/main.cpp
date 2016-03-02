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
#include "buttons.h"

App_t App;
// Dip Switch
#define DIPSWITCH_GPIO  GPIOA
#define DIPSWITCH_PIN1  8
#define DIPSWITCH_PIN2  11
#define DIPSWITCH_PIN3  12
#define DIPSWITCH_PIN4  15

Vibro_t Vibro(GPIOB, 8, TIM4, 3);
LedRGB_t Led({GPIOB, 1, TIM3, 4}, {GPIOB, 0, TIM3, 3}, {GPIOB, 5, TIM3, 2});

#if 1 // ============================ Timers ===================================
// Universal VirtualTimer callback
void TmrGeneralCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI((eventmask_t)p);
    chSysUnlockFromIsr();
}
// Check radio buffer
void TmrCheckCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI(EVTMSK_CHECK);
    chVTSetI(&App.TmrCheck, MS2ST(CHECK_PERIOD_MS), TmrCheckCallback, nullptr);
    chSysUnlockFromIsr();
}

#define SetTmr15min()   chVTSet(&Tmr15, S2ST(MINUTES_15_S), TmrGeneralCallback, (void*)EVTMSK_15MIN);
#define SetTmr45min()   chVTSet(&Tmr45, S2ST(MINUTES_45_S), TmrGeneralCallback, (void*)EVTMSK_45MIN);
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

    Uart.Init(115200);
    Uart.Printf("\r%S_%S  ID=%d", APP_NAME, APP_VERSION, App.ID);

    // Timers
    chVTSet(&App.TmrCheck, MS2ST(CHECK_PERIOD_MS), TmrCheckCallback, nullptr);

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
        if(EvtMsk & EVTMSK_CHECK) {
            // Get mode
            uint8_t b = GetDipSwitch();
            if((b & 0b0111) > 0b0100) b = 0b1111;   // Bad mode
            Mode_t NewMode = static_cast<Mode_t>(b);
            if(Mode != NewMode) {
                Mode = NewMode;
                if(Mode == mRx or Mode == mBadMode) TxState = tosOff;
                else TxState = tosOn;
                Led.Stop();
                Vibro.Stop();
                VibroSequence = nullptr;
                // Stop timers
                chVTReset(&Tmr15);
                chVTReset(&Tmr45);
                // Restart timer15 if needed
                if(Mode == mTx1515Hi or Mode == mTx1515Lo) SetTmr15min();
                Uart.Printf("\rMode=%u", Mode);
            }
            // Indicate led
            switch(Mode) {
                case mRxTxOffLo:
                case mRxTxOffHi:
                case mTx1515Lo:
                case mTx1515Hi:
                    switch(TxState) {
                        case tosOn:  Led.StartSequence(lsqModesTable[static_cast<uint32_t>(Mode)]); break;
                        case tosOff: Led.StartSequence(lsqTxDisabled); break;
                        case tosOnAndSwitchOffDisabled: Led.StartSequence(lsqRxTxOffSwitchOffDisabled); break;
                    }
                    break;

                default:
                    Led.StartSequence(lsqModesTable[static_cast<uint32_t>(Mode)]);
                    break;
            } // switch
            // Indicate vibro
            if(VibroSequence != nullptr) Vibro.StartSequence(VibroSequence);
        } // if EVTMSK_EVERY_SECOND
#endif

#if 1 // ==== Button ====
        if(EvtMsk & EVTMSK_BUTTONS) {
            BtnEvtInfo_t EInfo;
            while(BtnGetEvt(&EInfo) == OK) {
                if(EInfo.Type == bePress) {
                    Uart.Printf("Btn\r");
                    // Only RxTxOff reacts on button
                    if(Mode == mRxTxOffLo or Mode == mRxTxOffHi) {
                        switch(TxState) {
                            case tosOn:   // May switch off
                                TxState = tosOff;
                                SetTmr15min();
                                break;
                            case tosOff:  // Already off
                            case tosOnAndSwitchOffDisabled:   // Was off, may not off now
                                Led.StartSequence(lsqFailure);
                                break;
                        } // switch
                    } // if mode
                } // if press
            } // while getinfo ok
        } // EVTMSK_BTN_PRESS
#endif

#if 1 // ==== 15 min timer ====
        if(EvtMsk & EVTMSK_15MIN) {
            if(Mode == mRxTxOffLo or Mode == mRxTxOffHi) {
                // Will be here after 15 min of silence; switch Tx on
                TxState = tosOnAndSwitchOffDisabled;
                SetTmr45min();  // will be able to switch off after 45 min of tx
            }
            else if(Mode == mTx1515Lo or Mode == mTx1515Hi) {
                // Toggle Tx
                if(TxState == tosOn) TxState = tosOff;
                else TxState = tosOn;
                SetTmr15min();  // Restart timer
            }
        }
#endif

#if 1 // ==== 45 min timer ====
        if(EvtMsk & EVTMSK_45MIN) { // Will be here after 45 min of tx-off-disabled
            TxState = tosOn;
        }
#endif

    } // while true
}

void App_t::CheckRxTable() {
    // Get number of distinct received IDs and clear table
    chSysLock();
    uint32_t Cnt = Radio.RxTable.GetCount();
    Radio.RxTable.Clear();
    chSysUnlock();
    Uart.Printf("\rCnt = %u", Cnt);
    // ==== Select indication mode depending on received cnt ====
    switch(App.Mode) {
        case mRx:
        case mRxTxLo:
        case mRxTxHi:
        case mRxTxOffLo:
        case mRxTxOffHi:
            switch(Cnt) {
                case 0:  VibroSequence = nullptr;   break;
                case 1:  VibroSequence = vsqSingle; break;
                case 2:  VibroSequence = vsqPair;   break;
                default: VibroSequence = vsqMany;   break; // RxTable.Cnt  > 2
            }
            break;

        default:
            // TxLo, TxHi, Tx1515Lo, Tx1515Hi
            break;
    } // switch
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
