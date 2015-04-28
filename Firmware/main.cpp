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
//#include "radio_lvl1.h"

App_t App;
//Beeper_t Beeper;
Vibro_t Vibro;
LedRGB_t Led({GPIOB, 1, TIM3, 4}, {GPIOB, 0, TIM3, 3}, {GPIOB, 5, TIM3, 2});

// Universal VirtualTimer callback
void TmrGeneralCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI((eventmask_t)p);
    chSysUnlockFromIsr();
}

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
    Clk.UpdateFreqValues();

    // ==== Init OS ====
    halInit();
    chSysInit();

    // ==== Init Hard & Soft ====
    Uart.Init(115200);
    Uart.Printf("\rLocket AHB=%u", Clk.AHBFreqHz);

    App.InitThread();

//    App.LoadSettings();

//    PinSensors.Init();
//    Beeper.Init();
//    Beeper.StartSequence(bsqBeepBeep);

    Led.Init();
    Led.StartSequence(lsqStart);
//    Vibro.Init();
//    Vibro.StartSequence(vsqBrrBrr);
//    PinSetupOut(GPIOB, 8, omPushPull, pudNone);
//    PinClear(GPIOB, 8);

//    PwmPin_t IPin;
//    IPin.Init(GPIOB, 8, TIM4, 3, 22);
//    IPin.SetFreqHz(450);
//    IPin.Set(11);

//    Radio.Init();

    // Main cycle
    App.ITask();
}

__attribute__ ((__noreturn__))
void App_t::ITask() {
    while(true) {
        chThdSleepMilliseconds(999);
//        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
#if 0 // ==== Buttons ====
        if(EvtMsk & EVTMSK_BUTTONS) {
            BtnEvtInfo_t EInfo;
            while(ButtonEvtBuf.Get(&EInfo) == OK) {
//                Uart.Printf("\rEinfo: %u, %u,  %A", EInfo.Type, EInfo.BtnCnt, EInfo.BtnID, EInfo.BtnCnt, '-');
                Beeper.StartSequence(bsqButton);
                // Switch backlight on
                Lcd.Backlight(81);
                chVTRestart(&ITmrBacklight, MS2ST(4500), EVTMSK_BCKLT_OFF);
                // Process buttons
                switch(EInfo.BtnID[0]) {
                    case btnLTop:   // Iterate IDs
                        if(Settings.ID < ID_MAX) Settings.ID++;
                        else Settings.ID = ID_MIN;
                        SettingsHasChanged = true;
                        SaveSettings();
                        Interface.ShowID();
                        break;

                    case btnLBottom: // Deadtime on/off
                        Settings.DeadtimeEnabled = !Settings.DeadtimeEnabled;
                        SettingsHasChanged = true;
                        SaveSettings();
                        Interface.ShowDeadtimeSettings();
                        break;

                    case btnRTop:
                        if(Settings.DurationActive_s < DURATION_ACTIVE_MAX_S) {
                            Settings.DurationActive_s += 10;
                            SettingsHasChanged = true;
                            SaveSettings();
                            Interface.ShowDurationActive();
                        }
                        break;

                    case btnRBottom:
                        if(Settings.DurationActive_s > DURATION_ACTIVE_MIN_S) {
                            Settings.DurationActive_s -= 10;
                            SettingsHasChanged = true;
                            SaveSettings();
                            Interface.ShowDurationActive();
                        }
                        break;

                    default: break;
                } // switch
            } // while get
        } // if buttons
#endif

#if 0 // ==== Radio ====
        if(EvtMsk & EVTMSK_RADIO_RX) {
//            Uart.Printf("\rRadioRx");
            RadioIsOn = true;
            Interface.ShowRadio();
            chVTRestart(&ITmrRadioTimeout, S2ST(RADIO_NOPKT_TIMEOUT_S), EVTMSK_RADIO_ON_TIMEOUT);
            IProcessLedLogic();
            // Radio RX disables Deadtime if it is on
            DeadTimeIsNow = false;
            chVTReset(&ITmrDeadTime);
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
