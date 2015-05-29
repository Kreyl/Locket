/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    STM32F0xx/hal_lld.h
 * @brief   STM32F0xx HAL subsystem low level driver header.
 * @pre     This module requires the following macros to be defined in the
 *          @p board.h file:
 *          - STM32_LSECLK.
 *          - STM32_LSEDRV.
 *          - STM32_LSE_BYPASS (optionally).
 *          - STM32_HSECLK.
 *          - STM32_HSE_BYPASS (optionally).
 *          .
 *          One of the following macros must also be defined:
 *          - STM32F030 for Value Line devices.
 *          - STM32F0XX_LD for Low Density devices.
 *          - STM32F0XX_MD for Medium Density devices.
 *          .
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _HAL_LLD_H_
#define _HAL_LLD_H_

#include "stm32.h"
#include "stm32_registry.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Defines the support for realtime counters in the HAL.
 */
#define HAL_IMPLEMENTS_COUNTERS FALSE

/**
 * @name    Platform identification macros
 * @{
 */
#if defined(STM32F0XX_MD) || defined(__DOXYGEN__)
#define PLATFORM_NAME           "STM32F051xx/F061xx Entry Level Medium Density devices"
#define STM32F0XX

#elif defined(STM32F0XX_LD)
#define PLATFORM_NAME           "STM32F050xx/F060xx Entry Level Low Density devices"
#define STM32F0XX

#elif defined(STM32F030)
#define PLATFORM_NAME           "STM32F030xx Entry Level Value Line devices"
#define STM32F0XX

#else
#error "STM32F0xx device not specified"
#endif
/** @} */

/**
 * @name    Absolute Maximum Ratings
 * @{
 */
/**
 * @brief   Maximum system clock frequency.
 */
#define STM32_SYSCLK_MAX        48000000

/**
 * @brief   Maximum HSE clock frequency.
 */
#define STM32_HSECLK_MAX        32000000

/**
 * @brief   Minimum HSE clock frequency.
 */
#define STM32_HSECLK_MIN        1000000

/**
 * @brief   Maximum LSE clock frequency.
 */
#define STM32_LSECLK_MAX        1000000

/**
 * @brief   Minimum LSE clock frequency.
 */
#define STM32_LSECLK_MIN        32768

/**
 * @brief   Maximum PLLs input clock frequency.
 */
#define STM32_PLLIN_MAX         25000000

/**
 * @brief   Minimum PLLs input clock frequency.
 */
#define STM32_PLLIN_MIN         1000000

/**
 * @brief   Maximum PLL output clock frequency.
 */
#define STM32_PLLOUT_MAX        48000000

/**
 * @brief   Minimum PLL output clock frequency.
 */
#define STM32_PLLOUT_MIN        16000000

/**
 * @brief   Maximum APB clock frequency.
 */
#define STM32_PCLK_MAX          48000000

/**
 * @brief   Maximum ADC clock frequency.
 */
#define STM32_ADCCLK_MAX        14000000
/** @} */

/**
 * @name    Internal clock sources
 * @{
 */
#define STM32_HSICLK            8000000     /**< High speed internal clock. */
#define STM32_HSI14CLK          14000000    /**< 14MHz speed internal clock.*/
#define STM32_LSICLK            40000       /**< Low speed internal clock.  */
/** @} */

/**
 * @name    PWR_CR register bits definitions
 * @{
 */
#define STM32_PLS_MASK          (7 << 5)    /**< PLS bits mask.             */
#define STM32_PLS_LEV0          (0 << 5)    /**< PVD level 0.               */
#define STM32_PLS_LEV1          (1 << 5)    /**< PVD level 1.               */
#define STM32_PLS_LEV2          (2 << 5)    /**< PVD level 2.               */
#define STM32_PLS_LEV3          (3 << 5)    /**< PVD level 3.               */
#define STM32_PLS_LEV4          (4 << 5)    /**< PVD level 4.               */
#define STM32_PLS_LEV5          (5 << 5)    /**< PVD level 5.               */
#define STM32_PLS_LEV6          (6 << 5)    /**< PVD level 6.               */
#define STM32_PLS_LEV7          (7 << 5)    /**< PVD level 7.               */
/** @} */

/**
 * @name    RCC_CFGR register bits definitions
 * @{
 */
#define STM32_SW_HSI            (0 << 0)    /**< SYSCLK source is HSI.      */
#define STM32_SW_HSE            (1 << 0)    /**< SYSCLK source is HSE.      */
#define STM32_SW_PLL            (2 << 0)    /**< SYSCLK source is PLL.      */

#define STM32_HPRE_DIV1         (0 << 4)    /**< SYSCLK divided by 1.       */
#define STM32_HPRE_DIV2         (8 << 4)    /**< SYSCLK divided by 2.       */
#define STM32_HPRE_DIV4         (9 << 4)    /**< SYSCLK divided by 4.       */
#define STM32_HPRE_DIV8         (10 << 4)   /**< SYSCLK divided by 8.       */
#define STM32_HPRE_DIV16        (11 << 4)   /**< SYSCLK divided by 16.      */
#define STM32_HPRE_DIV64        (12 << 4)   /**< SYSCLK divided by 64.      */
#define STM32_HPRE_DIV128       (13 << 4)   /**< SYSCLK divided by 128.     */
#define STM32_HPRE_DIV256       (14 << 4)   /**< SYSCLK divided by 256.     */
#define STM32_HPRE_DIV512       (15 << 4)   /**< SYSCLK divided by 512.     */

#define STM32_PPRE_DIV1         (0 << 8)    /**< HCLK divided by 1.         */
#define STM32_PPRE_DIV2         (4 << 8)    /**< HCLK divided by 2.         */
#define STM32_PPRE_DIV4         (5 << 8)    /**< HCLK divided by 4.         */
#define STM32_PPRE_DIV8         (6 << 8)    /**< HCLK divided by 8.         */
#define STM32_PPRE_DIV16        (7 << 8)    /**< HCLK divided by 16.        */

#define STM32_ADCPRE_DIV2       (0 << 14)   /**< PCLK divided by 2.         */
#define STM32_ADCPRE_DIV4       (1 << 14)   /**< PCLK divided by 4.         */

#define STM32_PLLSRC_HSI        (0 << 16)   /**< PLL clock source is HSI.   */
#define STM32_PLLSRC_HSE        (1 << 16)   /**< PLL clock source is HSE.   */

#define STM32_MCOSEL_NOCLOCK    (0 << 24)   /**< No clock on MCO pin.       */
#define STM32_MCOSEL_HSI14      (3 << 24)   /**< HSI14 clock on MCO pin.    */
#define STM32_MCOSEL_SYSCLK     (4 << 24)   /**< SYSCLK on MCO pin.         */
#define STM32_MCOSEL_HSI        (5 << 24)   /**< HSI clock on MCO pin.      */
#define STM32_MCOSEL_HSE        (6 << 24)   /**< HSE clock on MCO pin.      */
#define STM32_MCOSEL_PLLDIV2    (7 << 24)   /**< PLL/2 clock on MCO pin.    */
/** @} */

/**
 * @name    RCC_BDCR register bits definitions
 * @{
 */
#define STM32_RTCSEL_MASK       (3 << 8)    /**< RTC clock source mask.     */
#define STM32_RTCSEL_NOCLOCK    (0 << 8)    /**< No clock.                  */
#define STM32_RTCSEL_LSE        (1 << 8)    /**< LSE used as RTC clock.     */
#define STM32_RTCSEL_LSI        (2 << 8)    /**< LSI used as RTC clock.     */
#define STM32_RTCSEL_HSEDIV     (3 << 8)    /**< HSE divided by 32 used as
                                                 RTC clock.                 */
/** @} */

/**
 * @name    RCC_CFGR3 register bits definitions
 * @{
 */
#define STM32_USART1SW_MASK     (3 << 0)    /**< USART1 clock source mask.  */
#define STM32_USART1SW_PCLK     (0 << 0)    /**< USART1 clock is PCLK.      */
#define STM32_USART1SW_SYSCLK   (1 << 0)    /**< USART1 clock is SYSCLK.    */
#define STM32_USART1SW_LSE      (2 << 0)    /**< USART1 clock is LSE.       */
#define STM32_USART1SW_HSI      (3 << 0)    /**< USART1 clock is HSI.       */
#define STM32_I2C1SW_MASK       (1 << 4)    /**< I2C clock source mask.     */
#define STM32_I2C1SW_HSI        (0 << 4)    /**< I2C clock is HSI.          */
#define STM32_I2C1SW_SYSCLK     (1 << 4)    /**< I2C clock is SYSCLK.       */
#define STM32_CECSW_MASK        (1 << 6)    /**< CEC clock source mask.     */
#define STM32_CECSW_HSI         (0 << 6)    /**< CEC clock is HSI/244.      */
#define STM32_CECSW_LSE         (1 << 6)    /**< CEC clock is LSE.          */
#define STM32_ADCSW_MASK        (1 << 8)    /**< ADC clock source mask.     */
#define STM32_ADCSW_HSI14       (0 << 8)    /**< ADC clock is HSI14.        */
#define STM32_ADCSW_PCLK        (1 << 8)    /**< ADC clock is PCLK/2|4.     */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   Disables the PWR/RCC initialization in the HAL.
 */
#if !defined(STM32_NO_INIT) || defined(__DOXYGEN__)
#define STM32_NO_INIT               FALSE
#endif

/**
 * @brief   Enables or disables the programmable voltage detector.
 */
#if !defined(STM32_PVD_ENABLE) || defined(__DOXYGEN__)
#define STM32_PVD_ENABLE            FALSE
#endif

/**
 * @brief   Sets voltage level for programmable voltage detector.
 */
#if !defined(STM32_PLS) || defined(__DOXYGEN__)
#define STM32_PLS                   STM32_PLS_LEV0
#endif


/*
 * Configuration-related checks.
 */
#if !defined(STM32F0xx_MCUCONF)
#error "Using a wrong mcuconf.h file, STM32F0xx_MCUCONF not defined"
#endif

/* STM32 ISR, DMA and RCC helpers.*/
#include "stm32_isr.h"
#include "stm32_dma.h"
#include "stm32_rcc.h"

#ifdef __cplusplus
extern "C" {
#endif
  void hal_lld_init(void);
  void stm32_clock_init(void);
#ifdef __cplusplus
}
#endif

#endif /* _HAL_LLD_H_ */

/** @} */
