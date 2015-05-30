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
//Vibro_t Vibro(GPIOB, 8, TIM4, 3);
LedRGB_t Led({GPIOB, 1, TIM3, 4}, {GPIOB, 0, TIM3, 3}, {GPIOB, 5, TIM3, 2});

#if 1 // ============================ Timers ===================================
void TmrSecondCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI(EVTMSK_EVERY_SECOND);
    chVTSetI(&App.TmrSecond, MS2ST(1000), TmrSecondCallback, nullptr);
    chSysUnlockFromIsr();
}
#endif

int main(void) {
    // ==== Init Vcore & clock system ====
//    SetupVCore(vcore1V2);
    Clk.UpdateFreqValues();

    // ==== Init OS ====
    halInit();
    chSysInit();

    // ==== Init Hard & Soft ====
    App.InitThread();
//    App.ID = App.EE.Read32(EE_DEVICE_ID_ADDR);  // Read device ID
    chVTSet(&App.TmrSecond, MS2ST(1000), TmrSecondCallback, nullptr);

    Uart.Init(115200);
    Uart.Printf("\r%S  ID=%d", VERSION_STRING, App.ID);

//    Beeper.Init();
//    Beeper.StartSequence(bsqBeepBeep);

    // Led
    Led.Init();

//    Vibro.Init();

    if(Radio.Init() != OK) Led.StartSequence(lsqFailure);
    else Led.StartSequence(lsqStart);

    // Main cycle
    App.ITask();
}



__attribute__ ((__noreturn__))
void App_t::ITask() {
    while(true) {
//        chThdSleepMilliseconds(999);
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
        // ==== Uart cmd ====
        if(EvtMsk & EVTMSK_UART_NEW_CMD) {
            OnUartCmd(&Uart);
            Uart.SignalCmdProcessed();
        }

        // ==== Every second ====
        if(EvtMsk & EVTMSK_EVERY_SECOND) {
            // Get mode
            uint8_t b = GetDipSwitch();
            b &= 0b00000111;    // Consider only lower bits
            Mode_t NewMode = static_cast<Mode_t>(b);
            if(Mode != NewMode) {
                if(Mode == mError) Led.StartSequence(lsqFailure);
                else {
                    Led.StartSequence(lsqStart);
                    Mode = NewMode;
                }
                chThdSleepMilliseconds(540);
            }
        }

#if 0 // ==== Radio ====
        if(EvtMsk & EVTMSK_RADIO_RX) {
            Uart.Printf("\rRadioRx");
            chVTRestart(&ITmrRadioTimeout, S2ST(RADIO_NOPKT_TIMEOUT_S), EVTMSK_RADIO_ON_TIMEOUT);
            if(Mode == mRxLight or Mode == mRxVibroLight)
        }
        if(EvtMsk & EVTMSK_RADIO_ON_TIMEOUT) {
//            Uart.Printf("\rRadioTimeout");
            RadioIsOn = false;
            Interface.ShowRadio();
            IProcessLedLogic();
        }
#endif

#if 0 // ==== Saving settings ====
        if(EvtMsk & EVTMSK_SAVE) { ISaveSettingsReally(); }
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
    Rslt ^= 0xF;
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN1);
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN2);
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN3);
    PinSetupAnalog(DIPSWITCH_GPIO, DIPSWITCH_PIN4);
    return Rslt;
}

uint8_t App_t::ISetID(int32_t NewID) {
//    if(NewID < ID_MIN or NewID > ID_MAX) return FAILURE;
//    uint8_t rslt = EE.Write32(EE_DEVICE_ID_ADDR, NewID);
//    if(rslt == OK) {
//        ID = NewID;
//        Uart.Printf("\rNew ID: %u", ID);
//        return OK;
//    }
//    else {
//        Uart.Printf("\rEE error: %u", rslt);
//        return FAILURE;
//    }
    return FAILURE;
}

#if 0 // ===================== Load/save settings ==============================
void App_t::LoadSettings() {
    if(EE_PTR->ID < ID_MIN or EE_PTR->ID > ID_MAX) Settings.ID = ID_DEFAULT;
    else Settings.ID = EE_PTR->ID;

    if(EE_PTR->DurationActive_s < DURATION_ACTIVE_MIN_S or
       EE_PTR->DurationActive_s > DURATION_ACTIVE_MAX_S
       ) {
        Settings.DurationActive_s = DURATION_ACTIVE_DEFAULT;
    }
    else Settings.DurationActive_s = EE_PTR->DurationActive_s;

    Settings.DeadtimeEnabled = EE_PTR->DeadtimeEnabled;

    SettingsHasChanged = false;
}

void App_t::SaveSettings() {
    chSysLock();
    if(chVTIsArmedI(&ITmrSaving)) chVTResetI(&ITmrSaving);  // Reset timer
    chVTSetEvtI(&ITmrSaving, S2ST(4), EVTMSK_SAVE);
    chSysUnlock();
}

void App_t::ISaveSettingsReally() {
    Flash_t::UnlockEE();
    chSysLock();
    uint8_t r = OK;
    uint32_t *Src = (uint32_t*)&Settings;
    uint32_t *Dst = (uint32_t*)EE_PTR;
    for(uint32_t i=0; i<SETTINGS_SZ32; i++) {
        r = Flash_t::WaitForLastOperation();
        if(r != OK) break;
        *Dst++ = *Src++;
    }
    Flash_t::LockEE();
    chSysUnlock();
    if(r == OK) {
        Uart.Printf("\rSettings saved");
        SettingsHasChanged = false;
        Interface.ShowID();
        Interface.ShowDurationActive();
        Interface.ShowDeadtimeSettings();
    }
    else {
        Uart.Printf("\rSettings saving failure");
        Interface.Error("Save failure");
    }
}
#endif
