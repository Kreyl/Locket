/*
 * beeper.h
 *
 *  Created on: 22 марта 2015 г.
 *      Author: Kreyl
 */

#ifndef KL_LIB_BEEPER_H_
#define KL_LIB_BEEPER_H_

#include "ChunkTypes.h"
#include "hal.h"

#define BEEP_TOP_VALUE   22

class Beeper_t : public BaseSequencer_t<BeepChunk_t> {
private:
    PwmPin_t IPin;
    void ISwitchOff() { IPin.Set(0); }
    SequencerLoopTask_t ISetup() {
        IPin.SetFreqHz(IPCurrentChunk->Freq_Hz);
        IPin.Set(IPCurrentChunk->Volume);
        IPCurrentChunk++;   // Always goto next
        return sltProceed;  // Always proceed
    }
public:
    Beeper_t() : BaseSequencer_t(), IPin() {}
    void Init() { IPin.Init(GPIOB, 3, TIM2, 2, BEEP_TOP_VALUE, invInverted); }
};


#endif /* KL_LIB_BEEPER_H_ */
