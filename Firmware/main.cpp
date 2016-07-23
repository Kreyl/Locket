/*
 * File:   main.cpp
 * Author: Kreyl
 * Project: Salem Box
 *
 * Created on Mar 22, 2015, 01:23
 */

#include "buttons.h"
#include "main.h"
#include "SimpleSensors.h"
#include "Sequences.h"
#include "led.h"
#include "vibro.h"
#include "radio_lvl1.h"


LedRGBChunk_t lsqOn[] = {
        {csSetup, 99, clRed},
        {csEnd}
};

LedRGBChunk_t lsqStart[] = {
        {csSetup, 360, clRed},
        {csSetup, 360, clBlack},
        {csEnd}
};

Thread *PThread;
Eeprom_t EE;

int32_t ID;
uint8_t ISetID(int32_t NewID);
// Eternal methods
void ReadIDfromEE();
uint8_t GetDipSwitch();
void appSignalEvt(eventmask_t Evt) {
    chSysLock();
    chEvtSignalI(PThread, Evt);
    chSysUnlock();
}
void appSignalEvtI(eventmask_t Evt) { chEvtSignalI(PThread, Evt); }
void OnUartCmd(Uart_t *PUart);


//Beeper_t Beeper;
Vibro_t Vibro(GPIOB, 8, TIM4, 3);
LedRGB_t Led { {GPIOB, 1, TIM3, 4}, {GPIOB, 0, TIM3, 3}, {GPIOB, 5, TIM3, 2} };

int main(void) {
#if 1 // ============================= Init ====================================
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
    Clk.UpdateFreqValues();

    // ==== Init OS ====
    halInit();
    chSysInit();

    // ==== Init Hard & Soft ====
    Uart.Init(115200);
    PThread = chThdSelf();

    Led.Init();
#if BTN_ENABLED
    PinSensors.Init();
#endif

    ReadIDfromEE();
    // Get color from ee
    lsqOn[0].Color.DWord32 = EE.Read32(EE_ADDR_COLOR);
    lsqStart[0].Color.DWord32 = lsqOn[0].Color.DWord32;

    Uart.Printf("\r%S_%S  ID=%d\r", APP_NAME, APP_VERSION, ID);

    Vibro.Init();
    Vibro.StartSequence(vsqBrr);

    if(Radio.Init() != OK) {
        Led.StartSequence(lsqFailure);
        chThdSleepMilliseconds(2700);
    }
    else Led.StartSequence(lsqStart);
#endif

    // ==== Main cycle ====
    while(true) {
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
#if 1   // ==== Uart cmd ====
        if(EvtMsk & EVT_UART_NEW_CMD) {
            OnUartCmd(&Uart);
            Uart.SignalCmdProcessed();
        }
#endif

        if(EvtMsk & EVT_BUTTONS) {
            BtnEvtInfo_t EInfo;
            while(BtnGetEvt(&EInfo) == OK) {
                if(EInfo.Type == bePress) {
//                    Uart.Printf("Press\r");
                    Led.StartSequence(lsqOn);
                }
                else if(EInfo.Type == beRepeat) {
//                    Uart.Printf("Repeat\r");
                    lsqOn[0].Color = GetNextTableColor();
                    Led.StartSequence(lsqOn);
                }
                else if(EInfo.Type == beRelease) {
//                    Uart.Printf("Release\r");
                    Led.StartSequence(lsqOff);
                    // Save color to EE
                    EE.Write32(EE_ADDR_COLOR, lsqOn[0].Color.DWord32);
                }
            }
        }

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

void OnUartCmd(Uart_t *PUart) {
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

void ReadIDfromEE() {
    ID = EE.Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(ID < ID_MIN or ID > ID_MAX) {
        Uart.Printf("\rUsing default ID");
        ID = ID_DEFAULT;
    }
}

uint8_t ISetID(int32_t NewID) {
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


uint8_t GetDipSwitch() {
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
