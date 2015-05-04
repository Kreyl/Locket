/*
 * buttons.h
 *
 *  Created on: 07.02.2013
 *      Author: kreyl
 */

#ifndef BTNS_H_
#define BTNS_H_

#include "hal.h"
#ifdef STM32F2XX
#include "kl_lib_f2xx.h"
#elif defined STM32L1XX_MD || defined STM32L1XX_HD
#include "kl_lib_L15x.h"
#endif
#include "kl_buf.h"

#include "PinSnsSettings.h"
#include "SimpleSensors.h"

#define BUTTONS_ENABLED FALSE
#if BUTTONS_ENABLED

// Select required events. BtnPress is a must.
#define BTN_RELEASE     FALSE
#define BTN_LONGPRESS   FALSE    // Send LongPress evt
#define BTN_REPEAT      TRUE    // Send Repeat evt
#define BTN_COMBO       FALSE   // Allow combo

#define BTN_REPEAT_PERIOD_MS        180
#define BTN_LONGPRESS_DELAY_MS      603
#define BTN_DELAY_BEFORE_REPEAT_MS  (BTN_REPEAT_PERIOD_MS + BTN_LONGPRESS_DELAY_MS)

#if BTN_COMBO
#define BTNS_EVT_Q_LEN              7   // Length of events' query
#else
#define BTNS_EVT_Q_LEN              1   // No need in queue if combo not allowed
#endif

// Select convenient names
enum BtnName_t {btnRTop=0, btnRBottom=1, btnLTop=2, btnLBottom=3};

// BtnEvent: contains info about event type, count of participating btns and array with btn IDs
enum BtnEvt_t {bePress, beLongPress, beRelease, beCancel, beRepeat, beCombo};
struct BtnEvtInfo_t {
    BtnEvt_t Type;
    uint8_t BtnCnt;
    uint8_t BtnID[BUTTONS_CNT];
};

// Define correct button behavior depending on schematic
#define BTN_PRESS_STATE         pssFalling
#define BTN_RELEASE_STATE       pssRising
#define BTN_HOLDDOWN_STATE      pssLo

// Events
extern CircBuf_t<BtnEvtInfo_t, BTNS_EVT_Q_LEN> ButtonEvtBuf;

#endif

#endif /* BTNS_H_ */
