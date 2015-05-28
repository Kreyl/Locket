/*
 * kl_lib_f0xx.h
 *
 *  Created on: 10.12.2012
 *      Author: kreyl
 */

#ifndef KL_LIB_F072_H_
#define KL_LIB_F072_H_

#include <clocking_f030.h>
#include "stm32f0xx.h"
#include "ch.h"
#include "hal.h"
#include <cstdlib>

extern "C" {
//void _init(void);   // Need to calm linker
void __attribute__ ((weak)) _init(void)  {}
} // Extern "C"

// =============================== General =====================================
#define PACKED __attribute__ ((__packed__))
#ifndef countof
#define countof(A)  (sizeof(A)/sizeof(A[0]))
#endif

// IRQ priorities
#define IRQ_PRIO_LOW            15  // Minimum
#define IRQ_PRIO_MEDIUM         9
#define IRQ_PRIO_HIGH           7
#define IRQ_PRIO_VERYHIGH       4   // Higher than systick

// DMA
#define DMA_PRIORITY_LOW        STM32_DMA_CR_PL(0b00)
#define DMA_PRIORITY_MEDIUM     STM32_DMA_CR_PL(0b01)
#define DMA_PRIORITY_HIGH       STM32_DMA_CR_PL(0b10)
#define DMA_PRIORITY_VERYHIGH   STM32_DMA_CR_PL(0b11)

// Return values
#define OK              0
#define FAILURE         1
#define TIMEOUT         2
#define BUSY            3
#define NEW             4
#define IN_PROGRESS     5
#define LAST            6
#define CMD_ERROR       7
#define WRITE_PROTECT   8
#define CMD_UNKNOWN     9
#define EMPTY_STRING    10
#define NOT_A_NUMBER    11

typedef void (*ftVoidVoid)(void);

enum BitOrder_t {boMSB, boLSB};

class Convert {
public:
    static void U16ToArrAsBE(uint8_t *PArr, uint16_t N) {
        uint8_t *p8 = (uint8_t*)&N;
        *PArr++ = *(p8 + 1);
        *PArr   = *p8;
    }
    static void U32ToArrAsBE(uint8_t *PArr, uint32_t N) {
        uint8_t *p8 = (uint8_t*)&N;
        *PArr++ = *(p8 + 3);
        *PArr++ = *(p8 + 2);
        *PArr++ = *(p8 + 1);
        *PArr   = *p8;
    }
    static uint16_t ArrToU16AsBE(uint8_t *PArr) {
        uint16_t N;
        uint8_t *p8 = (uint8_t*)&N;
        *p8++ = *(PArr + 1);
        *p8 = *PArr;
        return N;
    }
    static uint32_t ArrToU32AsBE(uint8_t *PArr) {
        uint32_t N;
        uint8_t *p8 = (uint8_t*)&N;
        *p8++ = *(PArr + 3);
        *p8++ = *(PArr + 2);
        *p8++ = *(PArr + 1);
        *p8 = *PArr;
        return N;
    }
//    static void U16ChangeEndianness(uint16_t *p) { *p = __REV16(*p); }
//    static void U32ChangeEndianness(uint32_t *p) { *p = __REV(*p); }
    static inline uint8_t TryStrToUInt32(char* S, uint32_t *POutput) {
        if(*S == '\0') return EMPTY_STRING;
        char *p;
        *POutput = strtoul(S, &p, 0);
        return (*p == 0)? OK : NOT_A_NUMBER;
    }
    static inline uint8_t TryStrToInt32(char* S, int32_t *POutput) {
        if(*S == '\0') return EMPTY_STRING;
        char *p;
        *POutput = strtol(S, &p, 0);
        return (*p == '\0')? OK : NOT_A_NUMBER;
    }
};

// ============================ Simple delay ===================================
static inline void DelayLoop(volatile uint32_t ACounter) { while(ACounter--); }
static inline void Delay_ms(uint32_t Ams) {
    volatile uint32_t __ticks = (Clk.AHBFreqHz / 4000) * Ams;
    DelayLoop(__ticks);
}

#if 1 // ===================== Single pin manipulations ========================
enum PinOutMode_t {
    omPushPull  = 0,
    omOpenDrain = 1
};
enum PinPullUpDown_t {
    pudNone = 0b00,
    pudPullUp = 0b01,
    pudPullDown = 0b10
};
enum PinSpeed_t {
    psLow    = 0b00,
    psMedium = 0b01,
    psHigh   = 0b11
};
enum PinAF_t {AF0=0, AF1=1, AF2=2, AF3=3, AF4=4, AF5=5, AF6=6, AF7=7};

// Set/clear
static inline void PinSet    (GPIO_TypeDef *PGpioPort, const uint16_t APinNumber) { PGpioPort->BSRRL = (uint32_t)(1<<APinNumber); }
static inline void PinClear  (GPIO_TypeDef *PGpioPort, const uint16_t APinNumber) { PGpioPort->BRR  = (uint32_t)(1<<APinNumber); }
static inline void PinToggle (GPIO_TypeDef *PGpioPort, const uint16_t APinNumber) { PGpioPort->ODR ^= (uint32_t)(1<<APinNumber); }
// Check state
static inline bool PinIsSet(GPIO_TypeDef *PGpioPort, const uint16_t APinNumber) { return (PGpioPort->IDR & (uint32_t)(1<<APinNumber)); }
// Setup
static inline void PinClockEnable(GPIO_TypeDef *PGpioPort) {
    if     (PGpioPort == GPIOA) RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    else if(PGpioPort == GPIOB) RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    else if(PGpioPort == GPIOC) RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    else if(PGpioPort == GPIOF) RCC->AHBENR |= RCC_AHBENR_GPIOFEN;
}
static inline void PinSetupOut(
        GPIO_TypeDef *PGpioPort,
        const uint16_t APinNumber,
        const PinOutMode_t PinOutMode,
        const PinPullUpDown_t APullUpDown,
        const PinSpeed_t ASpeed = psHigh
        ) {
    // Clock
    PinClockEnable(PGpioPort);
    // Setup mode
    PGpioPort->MODER &= ~(0b11 << (APinNumber*2));  // clear previous bits
    PGpioPort->MODER |= 0b01 << (APinNumber*2);     // Set new bits
    // Setup output type
    PGpioPort->OTYPER &= ~(1<<APinNumber);
    PGpioPort->OTYPER |= (uint32_t)PinOutMode << APinNumber;
    // Setup Pull-Up or Pull-Down
    PGpioPort->PUPDR &= ~(0b11 << (APinNumber*2)); // clear previous bits
    PGpioPort->PUPDR |= (uint32_t)APullUpDown << (APinNumber*2);
    // Setup speed
    PGpioPort->OSPEEDR &= ~(0b11 << (APinNumber*2)); // clear previous bits
    PGpioPort->OSPEEDR |= (uint32_t)ASpeed << (APinNumber*2);
}
static inline void PinSetupIn(GPIO_TypeDef *PGpioPort, const uint16_t APinNumber, const PinPullUpDown_t APullUpDown) {
    // Clock
    PinClockEnable(PGpioPort);
    // Setup mode
    PGpioPort->MODER &= ~(0b11 << (APinNumber*2)); // clear previous bits
    // Setup Pull-Up or Pull-Down
    PGpioPort->PUPDR &= ~(0b11 << (APinNumber*2)); // clear previous bits
    PGpioPort->PUPDR |= (uint32_t)APullUpDown << (APinNumber*2);
}
static inline void PinSetupAnalog(GPIO_TypeDef *PGpioPort, const uint16_t APinNumber) {
    // Clock
    PinClockEnable(PGpioPort);
    // Setup mode
    PGpioPort->MODER |= 0b11 << (APinNumber*2);  // Set new bits
    // Remove Pull-Up or Pull-Down
    PGpioPort->PUPDR &= ~(0b11 << (APinNumber*2)); // clear previous bits
}
static inline void PinSetupAlterFunc(
        GPIO_TypeDef *PGpioPort,
        const uint16_t APinNumber,
        const PinOutMode_t PinOutMode,
        const PinPullUpDown_t APullUpDown,
        const PinAF_t AAlterFunc,
        const PinSpeed_t ASpeed = psHigh
        ) {
    // Clock
    PinClockEnable(PGpioPort);
    // Setup mode
    PGpioPort->MODER &= ~(0b11 << (APinNumber*2));  // clear previous bits
    PGpioPort->MODER |= 0b10 << (APinNumber*2);     // Set new bits
    // Setup output type
    PGpioPort->OTYPER &= ~(1<<APinNumber);
    PGpioPort->OTYPER |= (uint32_t)PinOutMode << APinNumber;
    // Setup Pull-Up or Pull-Down
    PGpioPort->PUPDR &= ~(0b11 << (APinNumber*2)); // clear previous bits
    PGpioPort->PUPDR |= (uint32_t)APullUpDown << (APinNumber*2);
    // Setup speed
    PGpioPort->OSPEEDR &= ~(0b11 << (APinNumber*2)); // clear previous bits
    PGpioPort->OSPEEDR |= (uint32_t)ASpeed << (APinNumber*2);
    // Setup Alternate Function
    uint32_t n = (APinNumber <= 7)? 0 : 1;      // 0 if 0...7, 1 if 8..15
    uint32_t Shift = 4 * ((APinNumber <= 7)? APinNumber : APinNumber - 8);
    PGpioPort->AFR[n] &= ~(0b1111 << Shift);
    PGpioPort->AFR[n] |= (uint32_t)AAlterFunc << Shift;
}

#if 1 // ==== External IRQ ====
enum ExtiTrigType_t {ttRising, ttFalling, ttRisingFalling};

class PinIrq_t {
private:
    uint32_t IIrqChnl;
    GPIO_TypeDef *IGPIO;
    uint8_t IPinNumber;
public:
    void SetTriggerType(ExtiTrigType_t ATriggerType) {
        uint32_t IrqMsk = 1 << IPinNumber;
        switch(ATriggerType) {
            case ttRising:
                EXTI->RTSR |=  IrqMsk;  // Rising trigger enabled
                EXTI->FTSR &= ~IrqMsk;  // Falling trigger disabled
                break;
            case ttFalling:
                EXTI->RTSR &= ~IrqMsk;  // Rising trigger disabled
                EXTI->FTSR |=  IrqMsk;  // Falling trigger enabled
                break;
            case ttRisingFalling:
                EXTI->RTSR |=  IrqMsk;  // Rising trigger enabled
                EXTI->FTSR |=  IrqMsk;  // Falling trigger enabled
                break;
        } // switch
    }

    void Setup(GPIO_TypeDef *GPIO, const uint8_t APinNumber, ExtiTrigType_t ATriggerType) {
        IGPIO = GPIO;
        IPinNumber = APinNumber;
        rccEnableAPB2(RCC_APB2ENR_SYSCFGEN, FALSE); // Enable sys cfg controller
        // Connect EXTI line to the pin of the port
        uint8_t Indx   = APinNumber / 4;            // Indx of EXTICR register
        uint8_t Offset = (APinNumber & 0x03) * 4;   // Offset in EXTICR register
        SYSCFG->EXTICR[Indx] &= ~((uint32_t)0b1111 << Offset);  // Clear port-related bits
        // GPIOA requires all zeroes => nothing to do in this case
        if     (GPIO == GPIOB) SYSCFG->EXTICR[Indx] |= (uint32_t)0b0001 << Offset;
        else if(GPIO == GPIOC) SYSCFG->EXTICR[Indx] |= (uint32_t)0b0010 << Offset;
        // Configure EXTI line
        uint32_t IrqMsk = 1 << APinNumber;
        EXTI->IMR  |=  IrqMsk;      // Interrupt mode enabled
        EXTI->EMR  &= ~IrqMsk;      // Event mode disabled
        SetTriggerType(ATriggerType);
        EXTI->PR    =  IrqMsk;      // Clean irq flag
        // Get IRQ channel
        if     ((APinNumber == 0) or (APinNumber == 1)) IIrqChnl = EXTI0_1_IRQn;
        else if((APinNumber == 2) or (APinNumber == 3)) IIrqChnl = EXTI2_3_IRQn;
        else IIrqChnl = EXTI4_15_IRQn;
    }
    void EnableIrq(const uint32_t Priority) { nvicEnableVector(IIrqChnl, CORTEX_PRIORITY_MASK(Priority)); }
    void DisableIrq() { nvicDisableVector(IIrqChnl); }
    void CleanIrqFlag() { EXTI->PR = (1 << IPinNumber); }
};
#endif // EXTI
#endif

#if 1 // ============================== Timers =================================
enum TmrTrigInput_t {tiITR0=0x00, tiITR1=0x10, tiITR2=0x20, tiITR3=0x30, tiTIED=0x40, tiTI1FP1=0x50, tiTI2FP2=0x60, tiETRF=0x70};
enum TmrMasterMode_t {mmReset=0x00, mmEnable=0x10, mmUpdate=0x20, mmComparePulse=0x30, mmCompare1=0x40, mmCompare2=0x50, mmCompare3=0x60, mmCompare4=0x70};
enum TmrSlaveMode_t {smDisable=0, smEncoder1=1, smEncoder2=2, smEncoder3=3, smReset=4, smGated=5, smTrigger=6, smExternal=7};
enum Inverted_t {invNotInverted, invInverted};
enum ExtTrigPol_t {etpInverted=0x8000, etpNotInverted=0x0000};
enum ExtTrigPsc_t {etpOff=0x0000, etpDiv2=0x1000, etpDiv4=0x2000, etpDiv8=0x30000};

class Timer_t {
private:
    TIM_TypeDef* ITmr;
    uint32_t *PClk;
public:
    volatile uint32_t *PCCR;    // Made public to allow DMA
    // Common
    void Init(TIM_TypeDef* Tmr);
    void Deinit();
    void Enable()  { ITmr->CR1 |=  TIM_CR1_CEN; }
    void Disable() { ITmr->CR1 &= ~TIM_CR1_CEN; }
    void SetTopValue(uint16_t Value) { ITmr->ARR = Value; }
    void SetCounter(uint16_t Value) { ITmr->CNT = Value; }
    uint16_t GetCounter() { return ITmr->CNT; }
    uint16_t GetTopValue() { return ITmr->ARR; }
    void SetPrescaler(uint16_t Value) { ITmr->PSC = Value; }
    void SetUpdateFreq(uint32_t FreqHz) {
        uint32_t ClkCnt = *PClk / (ITmr->PSC + 1);
        SetTopValue((ClkCnt / FreqHz) - 1);
    }
    void SetCounterFreq(uint32_t CounterFreqHz) { ITmr->PSC = (*PClk / CounterFreqHz) - 1; }
    // Master/Slave
    void SetTriggerInput(TmrTrigInput_t TrgInput) {
        uint16_t tmp = ITmr->SMCR;
        tmp &= ~TIM_SMCR_TS;   // Clear bits
        tmp |= (uint16_t)TrgInput;
        ITmr->SMCR = tmp;
    }
    void SelectMasterMode(TmrMasterMode_t MasterMode) {
        uint16_t tmp = ITmr->CR2;
        tmp &= ~TIM_CR2_MMS;
        tmp |= (uint16_t)MasterMode;
        ITmr->CR2 = tmp;
    }
    void SelectSlaveMode(TmrSlaveMode_t SlaveMode) {
        uint16_t tmp = ITmr->SMCR;
        tmp &= ~TIM_SMCR_SMS;
        tmp |= (uint16_t)SlaveMode;
        ITmr->SMCR = tmp;
    }
    void EnableExternalClk(ExtTrigPol_t ExtTrigPol = etpNotInverted, ExtTrigPsc_t ExtTrigPsc = etpOff) { ITmr->SMCR = (uint16_t)ExtTrigPol | (uint16_t)ExtTrigPsc | TIM_SMCR_ECE; }
    // DMA, Irq, Evt
    void EnableDmaOnTrigger() { ITmr->DIER |= TIM_DIER_TDE; }
    void GenerateUpdateEvt()  { ITmr->EGR = TIM_EGR_UG; }
    void EnableIrqOnUpdate()  { ITmr->DIER |= TIM_DIER_UIE; }
    void ClearIrqBits()       { ITmr->SR = 0; }
    // PWM
    void InitPwm(GPIO_TypeDef *GPIO, uint16_t N, PinAF_t AAlterFunc,
            uint8_t Chnl, Inverted_t Inverted,
            const PinSpeed_t ASpeed = psHigh);
    void InitPwmNegativeChnl(GPIO_TypeDef *GPIO, uint16_t N, PinAF_t AAlterFunc,
            uint8_t Chnl, Inverted_t Inverted,
            const PinSpeed_t ASpeed = psHigh);
    void SetPwm(uint16_t Value) { *PCCR = Value; }
};
#endif

#if 1 // ============================== SPI ====================================
enum CPHA_t {cphaFirstEdge, cphaSecondEdge};
enum CPOL_t {cpolIdleLow, cpolIdleHigh};
enum SpiBaudrate_t {
    sbFdiv2   = 0b000,
    sbFdiv4   = 0b001,
    sbFdiv8   = 0b010,
    sbFdiv16  = 0b011,
    sbFdiv32  = 0b100,
    sbFdiv64  = 0b101,
    sbFdiv128 = 0b110,
    sbFdiv256 = 0b111,
};

class Spi_t {
private:
    SPI_TypeDef *PSpi;
public:
    void Setup(SPI_TypeDef *Spi, BitOrder_t BitOrder,
            CPOL_t CPOL, CPHA_t CPHA, SpiBaudrate_t Baudrate) {
        PSpi = Spi;
        // Clocking
        if      (PSpi == SPI1) { rccEnableSPI1(FALSE); }
        else if (PSpi == SPI2) { rccEnableSPI2(FALSE); }
        // Mode: Master, NSS software controlled and is 1, 8bit, NoCRC, FullDuplex
        PSpi->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR;
        if(BitOrder == boLSB) PSpi->CR1 |= SPI_CR1_LSBFIRST;    // MSB/LSB
        if(CPOL == cpolIdleHigh) PSpi->CR1 |= SPI_CR1_CPOL;     // CPOL
        if(CPHA == cphaSecondEdge) PSpi->CR1 |= SPI_CR1_CPHA;   // CPHA
        PSpi->CR1 |= ((uint16_t)Baudrate) << 3;                 // Baudrate
        PSpi->CR2 = 0x0300;                                // 8 bit
        PSpi->I2SCFGR &= ~((uint16_t)SPI_I2SCFGR_I2SMOD);       // Disable I2S
    }
    void Enable () { PSpi->CR1 |=  SPI_CR1_SPE; }
    void Disable() { PSpi->CR1 &= ~SPI_CR1_SPE; }
    void EnableTxDma() { PSpi->CR2 |= SPI_CR2_TXDMAEN; }
    void WaitBsyHi2Lo() { while(PSpi->SR & SPI_SR_BSY); }
    uint8_t ReadWriteByte(uint8_t AByte) {
        PSpi->DR = AByte;
        while(!(PSpi->SR & SPI_SR_RXNE));  // Wait for SPI transmission to complete
        return PSpi->DR;
    }
};
#endif

#endif /* KL_LIB_F072_H_ */
