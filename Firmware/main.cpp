/*
 * File:   main.cpp
 * Author: Kreyl
 * Project: Salem Box
 *
 * Created on Mar 22, 2015, 01:23
 */

#include "main.h"
#include "Sequences.h"
#include "led.h"
#include "vibro.h"
#include "radio_lvl1.h"

App_t App;

// EEAddresses
#define EE_ADDR_DEVICE_ID       0
#define EE_ADDR_COLOR           4

// Dip Switch
#define DIPSWITCH_GPIO  GPIOA
#define DIPSWITCH_PIN1  8
#define DIPSWITCH_PIN2  11
#define DIPSWITCH_PIN3  12
#define DIPSWITCH_PIN4  15

Vibro_t Vibro {VIBRO_PIN};
LedRGB_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN };

static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static void ReadAndSetupMode();
static uint8_t ISetID(int32_t NewID);
Eeprom_t EE;
static void ReadIDfromEE();

#if 1 // ============================ Timers ===================================
static TmrKL_t TmrEverySecond {MS2ST(1000), EVT_EVERY_SECOND, tktPeriodic};
static TmrKL_t TmrIndication  {MS2ST(3600), EVT_INDICATION_OFF, tktOneShot};

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

    Uart.Init(115200, UART_GPIO, UART_TX_PIN, UART_GPIO, UART_RX_PIN);
    ReadIDfromEE();
    Uart.Printf("\r%S_%S  ID=%d\r", APP_NAME, APP_VERSION, App.ID);

    Led.Init();
    Vibro.Init();
    Vibro.StartSequence(vsqBrr);
    chThdSleepMilliseconds(180);
//    PinSensors.Init();

    TmrEverySecond.InitAndStart();
    TmrIndication.Init();
    App.SignalEvt(EVT_EVERY_SECOND); // check it now

    if(Radio.Init() == OK) Led.StartSequence(lsqStart);
    else Led.StartSequence(lsqFailure);
    chThdSleepMilliseconds(1008);

    // Main cycle
    App.ITask();
}

__noreturn
void App_t::ITask() {
    while(true) {
        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);
        if(Evt & EVT_EVERY_SECOND) {
            ReadAndSetupMode();
        }

        if(Evt & EVT_RADIO) {
            // Something received
            if(Mode == modeDetectorRx) {
                if(Led.IsIdle()) Led.StartSequence(lsqCyan);
                TmrIndication.Restart(); // Reset off timer
            }
        }

        if(Evt & EVT_INDICATION_OFF) Led.StartSequence(lsqOff);

#if 1   // ==== Uart cmd ====
        if(Evt & EVT_UART_NEW_CMD) {
            OnCmd((Shell_t*)&Uart);
            Uart.SignalCmdProcessed();
        }
#endif
    } // while true
}

void ReadAndSetupMode() {
    static uint8_t OldDipSettings = 0xFF;
    uint8_t b = GetDipSwitch();
    if(b == OldDipSettings) return;
    // Something has changed
    Radio.MustSleep = true; // Sleep until we decide what to do
    // Reset everything
    Vibro.Stop();
    Led.Stop();
    // Analyze switch
    OldDipSettings = b;
    Uart.Printf("Dip: %02X\r", b);
    // Get mode
    if(App.ID == 41 or App.ID == 43 or App.ID == 45 or App.ID == 47) {
        App.Mode = modeDetectorTx;
        Led.StartSequence(lsqDetectorTxMode);
        Radio.MustSleep = false;
    }
    else if(App.ID == 42 or App.ID == 44 or App.ID == 46 or App.ID == 48) {
        App.Mode = modeDetectorRx;
        Led.StartSequence(lsqDetectorRxMode);
        Radio.MustSleep = false;
    }
    else {
        App.Mode = modeNone;
        Led.StartSequence(lsqFailure);
    }
    // ==== Setup TX power ====
    uint8_t pwrIndx = (b & 0b0111);
    chSysLock();
    CC.SetTxPower(CCPwrTable[pwrIndx]);
    chSysUnlock();
}

void App_t::OnCmd(Shell_t *PShell) {
    Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(OK);
    }

    else if(PCmd->NameIs("GetID")) PShell->Reply("ID", App.ID);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNextNumber(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        uint8_t r = ISetID(dw32);
        PShell->Ack(r);
    }

    else PShell->Ack(CMD_UNKNOWN);
}

#if 1 // =========================== ID management =============================
void ReadIDfromEE() {
    App.ID = EE.Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(App.ID < ID_MIN or App.ID > ID_MAX) {
        Uart.Printf("\rUsing default ID\r");
        App.ID = ID_DEFAULT;
    }
}

uint8_t ISetID(int32_t NewID) {
    if(NewID < ID_MIN or NewID > ID_MAX) return FAILURE;
    uint8_t rslt = EE.Write32(EE_ADDR_DEVICE_ID, NewID);
    if(rslt == OK) {
        App.ID = NewID;
        Uart.Printf("New ID: %u\r", App.ID);
        return OK;
    }
    else {
        Uart.Printf("EE error: %u\r", rslt);
        return FAILURE;
    }
}
#endif

// ====== DIP switch ======
uint8_t GetDipSwitch() {
    uint8_t Rslt = 0;
    for(int i=0; i<DIP_SW_CNT; i++) PinSetupInput(DipSwPin[i].PGpio, DipSwPin[i].Pin, DipSwPin[i].PullUpDown);
    for(int i=0; i<DIP_SW_CNT; i++) {
        if(!PinIsHi(DipSwPin[i].PGpio, DipSwPin[i].Pin)) Rslt |= (1 << i);
        PinSetupAnalog(DipSwPin[i].PGpio, DipSwPin[i].Pin);
    }
    return Rslt;
}
