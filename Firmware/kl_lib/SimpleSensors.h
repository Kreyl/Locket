/*
 * SimpleSensors.h
 *
 *  Created on: 17 џэт. 2015 у.
 *      Author: Kreyl
 */

#ifndef KL_LIB_SIMPLESENSORS_H_
#define KL_LIB_SIMPLESENSORS_H_

/*
 * Simple sensors are sensors with two logic states: Low and High.
 * Every time state changes (edge occures) new event generated.
 * Single event generated per pin change.
 */

#include "hal.h"
#ifdef STM32F2XX
#include "kl_lib_f2xx.h"
#elif defined STM32L1XX_MD || defined STM32L1XX_HD
#include "kl_lib_L15x.h"
#endif
#include <PinSnsSettings.h>

class SimpleSensors_t {
private:
    PinSnsState_t States[PIN_SNS_CNT];
public:
    void Init();
    void Shutdown() { for(uint32_t i=0; i<PIN_SNS_CNT; i++) PinSns[i].Off(); }
    // Inner use
    void ITask();
};

extern SimpleSensors_t PinSensors;

#endif /* KL_LIB_SIMPLESENSORS_H_ */
