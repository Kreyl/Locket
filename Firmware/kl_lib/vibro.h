/*
 * vibro.h
 *
 *  Created on: 26-04-2015 ã.
 *      Author: Kreyl
 */

#ifndef KL_LIB_VIBRO_H_
#define KL_LIB_VIBRO_H_

#include "ChunkTypes.h"
#include "hal.h"

#define VIBRO_TOP_VALUE     22
#define VIBRO_FREQ_HZ       3600

class Vibro_t : public BaseSequencer_t<BaseChunk_t> {
private:
    PwmPin_t IPin;
    void ISwitchOff() { IPin.Set(0); }
    SequencerLoopTask_t ISetup() {
        IPin.Set(IPCurrentChunk->Volume);
        IPCurrentChunk++;   // Always goto next
        return sltProceed;  // Always proceed
    }
public:
    Vibro_t() : BaseSequencer_t(), IPin() {}
    void Init() {
        IPin.Init(GPIOB, 8, TIM4, 3, VIBRO_TOP_VALUE);
        IPin.SetFreqHz(VIBRO_FREQ_HZ);
    }
};


#endif /* KL_LIB_VIBRO_H_ */
