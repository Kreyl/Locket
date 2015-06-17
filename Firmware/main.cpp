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
// LED sequence indicating that someone is near
LedRGBChunk_t lsqIndicationOn[] = {
        {csSetup, 720, CLR_DEFAULT},
        {csEnd}
};
LedRGBChunk_t lsqShowColor[] = {
        {csSetup, 0, CLR_DEFAULT},
        {csEnd}
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
// Check radio buffer
void TmrIndicationCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI(EVTMSK_INDICATION);
    chVTSetI(&App.TmrIndication, MS2ST(INDICATION_PERIOD_MS), TmrIndicationCallback, nullptr);
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
    chVTSet(&App.TmrIndication, MS2ST(INDICATION_PERIOD_MS), TmrIndicationCallback, nullptr);

    Uart.Init(115200);
//    Beeper.Init();
//    Beeper.StartSequence(bsqBeepBeep);

    Led.Init();
    App.ReadColorFromEE();

    Uart.Printf("\r%S_%S  ID=%d   Color: %u %u %u", APP_NAME, APP_VERSION, App.ID,
                lsqIndicationOn[0].Color.R, lsqIndicationOn[0].Color.G, lsqIndicationOn[0].Color.B);

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
            if(Mode != NewMode or FirstTimeModeIndication) {
                FirstTimeModeIndication = false;
                Mode = NewMode;
                Led.Stop();
                Vibro.Stop();
                LightWasOn = false;
                Uart.Printf("\rMode=%u", Mode);
                // Indicate Mode
                Led.StartSequence(lsqModesTable[static_cast<uint32_t>(Mode)]);
            }
        } // if EVTMSK_EVERY_SECOND
#endif

#if 1   // ==== Indication ====
        if(EvtMsk & EVTMSK_INDICATION) {
            if(Mode == mColorSelect) {
                Color_t *pcl = &lsqIndicationOn[0].Color;
                // Select next color
                uint32_t ClrIndx=0;
                for(uint32_t i=0; i<CLRTABLE_CNT; i++) {
                    if(*pcl == ClrTable[i]) {
                        ClrIndx = i+1;
                        break;
                    }
                } // for
                if(ClrIndx >= CLRTABLE_CNT) ClrIndx = 0;
                *pcl = ClrTable[ClrIndx];
                SaveColorToEE();
                // Show selected
                lsqShowColor[0].Color = ClrTable[ClrIndx];
                Led.StartSequence(lsqShowColor);
            }
            // Transmitter
            else if(Mode >= mTxLowPwr and Mode <= mTxMaxPwr) Led.StartSequence(lsqTxOperating);
            // In other modes, check radio
            else CheckRxTable();
        } // if evtmsk
#endif
    } // while true
}

void App_t::CheckRxTable() {
    // Get number of distinct received IDs and clear table
    chSysLock();
    uint32_t Cnt = Radio.RxTable.GetCount();
    Radio.RxTable.Clear();
    chSysUnlock();
//    Uart.Printf("\rCnt = %u", Cnt);
    // ==== Select indication mode depending on received cnt ====
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
    } // if Cnt != 0
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

    else if(PCmd->NameIs("GetColor")) PUart->Printf("Color %d,%d,%d", lsqIndicationOn[0].Color.R, lsqIndicationOn[0].Color.G, lsqIndicationOn[0].Color.B);

    else if(PCmd->NameIs("SetColor")) {
        Color_t cl;
        if(PCmd->GetNextToken() != OK) { PUart->Ack(CMD_ERROR); return; }
        if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { PUart->Ack(CMD_ERROR); return; }
        cl.R = dw32;
        if(PCmd->GetNextToken() != OK) { PUart->Ack(CMD_ERROR); return; }
        if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { PUart->Ack(CMD_ERROR); return; }
        cl.G = dw32;
        if(PCmd->GetNextToken() != OK) { PUart->Ack(CMD_ERROR); return; }
        if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { PUart->Ack(CMD_ERROR); return; }
        cl.B = dw32;
        lsqIndicationOn[0].Color = cl;
        PUart->Ack(App.SaveColorToEE());
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

// Read color from EEPROM. Use default color if something strange is read.
void App_t::ReadColorFromEE() {
    Color_t *pcl = &lsqIndicationOn[0].Color;
    pcl->DWord32 = EE.Read32(EE_ADDR_COLOR);
    // Check if one of allowed colors
    for(uint32_t i=0; i<CLRTABLE_CNT; i++) {
        if(*pcl == ClrTable[i]) return;
    }
    *pcl = CLR_DEFAULT; // Allowed color not found, use default one
}
uint8_t App_t::SaveColorToEE() {
    Color_t *pcl = &lsqIndicationOn[0].Color;
    return EE.Write32(EE_ADDR_COLOR, pcl->DWord32);
}
