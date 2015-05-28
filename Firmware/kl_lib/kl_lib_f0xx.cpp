/*
 * kl_lib_f0.cpp
 *
 *  Created on: 10.12.2012
 *      Author: kreyl
 */

#include <kl_lib_f0xx.h>

#if 1 // ============================= Timer ===================================
void Timer_t::Init(TIM_TypeDef* Tmr) {
    ITmr = Tmr;
    if     (ITmr == TIM1)  { rccEnableTIM1(FALSE); }
    else if(ITmr == TIM2)  { rccEnableTIM2(FALSE); }
    else if(ITmr == TIM3)  { rccEnableTIM3(FALSE); }
    else if(ITmr == TIM6)  { rccEnableAPB1(RCC_APB1ENR_TIM6EN,  FALSE); }
    else if(ITmr == TIM7)  { rccEnableAPB1(RCC_APB1ENR_TIM7EN,  FALSE); }
    else if(ITmr == TIM14) { RCC->APB1ENR |= RCC_APB1ENR_TIM14EN; }
    else if(ITmr == TIM15) { RCC->APB2ENR |= RCC_APB2ENR_TIM15EN; }
    else if(ITmr == TIM16) { RCC->APB2ENR |= RCC_APB2ENR_TIM16EN; }
    else if(ITmr == TIM17) { RCC->APB2ENR |= RCC_APB2ENR_TIM17EN; }
    // Clock src
    PClk = &Clk.APBFreqHz;
}

void Timer_t::InitPwm(GPIO_TypeDef *GPIO, uint16_t N, PinAF_t AAlterFunc,
        uint8_t Chnl, Inverted_t Inverted,
        const PinSpeed_t ASpeed) {
    PinSetupAlterFunc(GPIO, N, omPushPull, pudNone, AAlterFunc, ASpeed);
    // Enable outputs for advanced timers
    ITmr->BDTR = TIM_BDTR_MOE | TIM_BDTR_AOE;
    // Output
    uint16_t tmp = (Inverted == invInverted)? 0b111 : 0b110; // PWM mode 1 or 2
    switch(Chnl) {
        case 1:
            PCCR = &ITmr->CCR1;
            ITmr->CCMR1 |= (tmp << 4);
            ITmr->CCER  |= TIM_CCER_CC1E;
            break;

        case 2:
            PCCR = &ITmr->CCR2;
            ITmr->CCMR1 |= (tmp << 12);
            ITmr->CCER  |= TIM_CCER_CC2E;
            break;

        case 3:
            PCCR = &ITmr->CCR3;
            ITmr->CCMR2 |= (tmp << 4);
            ITmr->CCER  |= TIM_CCER_CC3E;
            break;

        case 4:
            PCCR = &ITmr->CCR4;
            ITmr->CCMR2 |= (tmp << 12);
            ITmr->CCER  |= TIM_CCER_CC4E;
            break;

        default: break;
    }
}

void Timer_t::InitPwmNegativeChnl(GPIO_TypeDef *GPIO, uint16_t N, PinAF_t AAlterFunc,
            uint8_t Chnl, Inverted_t Inverted,
            const PinSpeed_t ASpeed) {
    PinSetupAlterFunc(GPIO, N, omPushPull, pudNone, AAlterFunc, ASpeed);
    // Output
    uint16_t tmp = (Inverted == invInverted)? 0b1100 : 0b0100;
    tmp <<= (Chnl - 1) * 4;
    ITmr->CCER |= tmp;
}
#endif

