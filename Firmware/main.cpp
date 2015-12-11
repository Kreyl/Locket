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

Vibro_t Vibro(GPIOB, 8, TIM4, 3);
LedRGB_t Led { {GPIOB, 1, TIM3, 4}, {GPIOB, 0, TIM3, 3}, {GPIOB, 5, TIM3, 2} };

#define BLINK_LEN_MS    99

const Color_t Clrs[3] = {clRed, clGreen, clBlue};

static WORKING_AREA(waIndicationThread, 256);
__attribute__((__noreturn__))
static void IndicationThread(void *arg) {
    chRegSetThreadName("Indication");
    while(true) {
        chThdSleepMilliseconds(1206);
        // Indicate Colors one by one
        bool VibroDone = false;
        uint32_t Cnt;
        for(uint8_t j=0; j<3; j++) {
            Cnt = Radio.RxTable[j].GetCount();
            if(Cnt != 0) {
                if(!VibroDone) Vibro.StartSequence(vsqBrrBrr);
                VibroDone = true;
                for(uint8_t i=0; i<Cnt; i++) {
                    Led.SetColor(Clrs[j]);
                    chThdSleepMilliseconds(BLINK_LEN_MS);
                    Led.SetColor(clBlack);
                    chThdSleepMilliseconds(BLINK_LEN_MS);
                }
                Radio.RxTable[j].Clear();
            }
        } // j
    } // while
}

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

    Led.Init();

    Uart.Printf("\r%S_%S\r", APP_NAME, APP_VERSION);

    Vibro.Init();
    Vibro.StartSequence(vsqBrr);

    if(Radio.Init() != OK) {
        Led.StartSequence(lsqFailure);
        chThdSleepMilliseconds(2700);
    }

    // Indication thread
    chThdCreateStatic(waIndicationThread, sizeof(waIndicationThread), NORMALPRIO, (tfunc_t)IndicationThread, NULL);

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

#if 0   // ==== Once a second ====
        if(EvtMsk & EVTMSK_SECOND) {
            DipToTxPwr();               // Check DIP switch
        }
#endif
    } // while true
}

void App_t::OnUartCmd(Uart_t *PUart) {
    UartCmd_t *PCmd = &PUart->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("\r%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) PUart->Ack(OK);

    else PUart->Ack(CMD_UNKNOWN);
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
