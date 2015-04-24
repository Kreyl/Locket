/*
 * main.h
 *
 *  Created on: 21 дек. 2014 г.
 *      Author: Kreyl
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "kl_lib_L15x.h"
#include "ch.h"
#include "hal.h"
#include "clocking_L1xx.h"
#include "evt_mask.h"
#include "uart.h"

#define VERSION_STRING  "v1.0"

// All values must be 32bit to make things easier
struct Settings_t {
    uint32_t ID;
    uint32_t DurationActive_s;
    union {
        uint32_t dummy32;   // force bool being 32-bit
        bool DeadtimeEnabled;
    };
};
#define SETTINGS_SZ32   (sizeof(Settings_t) / 4)
// EEPROM addresses
#define EE_ADDR     0
#define EE_PTR      (reinterpret_cast<Settings_t*>(EEPROM_BASE_ADDR + EE_ADDR))

// ==== Constants and default values ====
#define DURATION_ACTIVE_MIN_S   10
#define DURATION_ACTIVE_MAX_S   9990
#define DURATION_ACTIVE_DEFAULT 300
#define ID_MIN                  1
#define ID_MAX                  18
#define ID_DEFAULT              ID_MIN
#define DURATION_DEADTIME_S     90

// Radio timing
#define RADIO_NOPKT_TIMEOUT_S   4

class App_t {
private:
    bool RadioIsOn, LedBySnsMustBeOn, DeadTimeIsNow;
    VirtualTimer ITmrSaving, ITmrMSnsTimeout, ITmrRadioTimeout, ITmrBacklight, ITmrDeadTime;
    void ISaveSettingsReally();    // Really save settings
    Thread *PThread;
    void IProcessLedLogic();
    bool SettingsHasChanged;
public:
    void InitThread() { PThread = chThdSelf(); }
    Settings_t Settings;
    void LoadSettings();
    void SaveSettings();    // Prepare to save settings
    void SignalEvt(eventmask_t Evt) {
        chSysLock();
        chEvtSignalI(PThread, Evt);
        chSysUnlock();
    }
    void SignalEvtI(eventmask_t Evt) { chEvtSignalI(PThread, Evt); }
    // Inner use
    void ITask();
    App_t(): RadioIsOn(false), LedBySnsMustBeOn(false), DeadTimeIsNow(false),
            PThread(nullptr), SettingsHasChanged(false),
            Settings({ID_MIN, DURATION_ACTIVE_MIN_S, false}) {}
    friend class Interface_t;
};

extern App_t App;


#endif /* MAIN_H_ */
