#ifndef IWATCHDOG2_h
#define IWATCHDOG2_h

// IWatchdog - from https://github.com/stm32duino/Arduino_Core_STM32

#include <Arduino.h>
#include "config.h"
#include "checkconfig.h"
#include "debug.h"

//---------------------------------------------------------------------------------------

#define __STATIC_INLINE
#define UNUSED(x) (x);

/**
  * @brief Internal Low Speed oscillator (LSI) value.
  */
#if !defined  (LSI_VALUE)
#define LSI_VALUE               40000U     /*!< LSI Typical Value in Hz */
#endif /* LSI_VALUE */                     /*!< Value of the Internal Low Speed oscillator in Hz
                                                The real value may vary depending on the variations
                                                in voltage and temperature. */

/******************************************************************************/
/*                                                                            */
/*                        Independent WATCHDOG (IWDG)                         */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for IWDG_KR register  ********************/
#define IWDG_KR_KEY_Pos                     (0U)
#define IWDG_KR_KEY_Msk                     (0xFFFFUL << IWDG_KR_KEY_Pos)       /*!< 0x0000FFFF */
#define IWDG_KR_KEY                         IWDG_KR_KEY_Msk                    /*!< Key value (write only, read 0000h) */

/*******************  Bit definition for IWDG_PR register  ********************/
#define IWDG_PR_PR_Pos                      (0U)
#define IWDG_PR_PR_Msk                      (0x7UL << IWDG_PR_PR_Pos)           /*!< 0x00000007 */
#define IWDG_PR_PR                          IWDG_PR_PR_Msk                     /*!< PR[2:0] (Prescaler divider) */
#define IWDG_PR_PR_0                        (0x1UL << IWDG_PR_PR_Pos)           /*!< 0x00000001 */
#define IWDG_PR_PR_1                        (0x2UL << IWDG_PR_PR_Pos)           /*!< 0x00000002 */
#define IWDG_PR_PR_2                        (0x4UL << IWDG_PR_PR_Pos)           /*!< 0x00000004 */

/*******************  Bit definition for IWDG_RLR register  *******************/
#define IWDG_RLR_RL_Pos                     (0U)
#define IWDG_RLR_RL_Msk                     (0xFFFUL << IWDG_RLR_RL_Pos)        /*!< 0x00000FFF */
#define IWDG_RLR_RL                         IWDG_RLR_RL_Msk                    /*!< Watchdog counter reload value */

/*******************  Bit definition for IWDG_SR register  ********************/
#define IWDG_SR_PVU_Pos                     (0U)
#define IWDG_SR_PVU_Msk                     (0x1UL << IWDG_SR_PVU_Pos)          /*!< 0x00000001 */
#define IWDG_SR_PVU                         IWDG_SR_PVU_Msk                    /*!< Watchdog prescaler value update */
#define IWDG_SR_RVU_Pos                     (1U)
#define IWDG_SR_RVU_Msk                     (0x1UL << IWDG_SR_RVU_Pos)          /*!< 0x00000002 */
#define IWDG_SR_RVU                         IWDG_SR_RVU_Msk                    /*!< Watchdog counter reload value update */

/******************************************************************************/
/*                                                                            */
/*                         Window WATCHDOG (WWDG)                             */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for WWDG_CR register  ********************/
#define WWDG_CR_T_Pos                       (0U)
#define WWDG_CR_T_Msk                       (0x7FUL << WWDG_CR_T_Pos)           /*!< 0x0000007F */
#define WWDG_CR_T                           WWDG_CR_T_Msk                      /*!< T[6:0] bits (7-Bit counter (MSB to LSB)) */
#define WWDG_CR_T_0                         (0x01UL << WWDG_CR_T_Pos)           /*!< 0x00000001 */
#define WWDG_CR_T_1                         (0x02UL << WWDG_CR_T_Pos)           /*!< 0x00000002 */
#define WWDG_CR_T_2                         (0x04UL << WWDG_CR_T_Pos)           /*!< 0x00000004 */
#define WWDG_CR_T_3                         (0x08UL << WWDG_CR_T_Pos)           /*!< 0x00000008 */
#define WWDG_CR_T_4                         (0x10UL << WWDG_CR_T_Pos)           /*!< 0x00000010 */
#define WWDG_CR_T_5                         (0x20UL << WWDG_CR_T_Pos)           /*!< 0x00000020 */
#define WWDG_CR_T_6                         (0x40UL << WWDG_CR_T_Pos)           /*!< 0x00000040 */

/* Legacy defines */
#define  WWDG_CR_T0 WWDG_CR_T_0
#define  WWDG_CR_T1 WWDG_CR_T_1
#define  WWDG_CR_T2 WWDG_CR_T_2
#define  WWDG_CR_T3 WWDG_CR_T_3
#define  WWDG_CR_T4 WWDG_CR_T_4
#define  WWDG_CR_T5 WWDG_CR_T_5
#define  WWDG_CR_T6 WWDG_CR_T_6

#define WWDG_CR_WDGA_Pos                    (7U)
#define WWDG_CR_WDGA_Msk                    (0x1UL << WWDG_CR_WDGA_Pos)         /*!< 0x00000080 */
#define WWDG_CR_WDGA                        WWDG_CR_WDGA_Msk                   /*!< Activation bit */

/*******************  Bit definition for WWDG_CFR register  *******************/
#define WWDG_CFR_W_Pos                      (0U)
#define WWDG_CFR_W_Msk                      (0x7FUL << WWDG_CFR_W_Pos)          /*!< 0x0000007F */
#define WWDG_CFR_W                          WWDG_CFR_W_Msk                     /*!< W[6:0] bits (7-bit window value) */
#define WWDG_CFR_W_0                        (0x01UL << WWDG_CFR_W_Pos)          /*!< 0x00000001 */
#define WWDG_CFR_W_1                        (0x02UL << WWDG_CFR_W_Pos)          /*!< 0x00000002 */
#define WWDG_CFR_W_2                        (0x04UL << WWDG_CFR_W_Pos)          /*!< 0x00000004 */
#define WWDG_CFR_W_3                        (0x08UL << WWDG_CFR_W_Pos)          /*!< 0x00000008 */
#define WWDG_CFR_W_4                        (0x10UL << WWDG_CFR_W_Pos)          /*!< 0x00000010 */
#define WWDG_CFR_W_5                        (0x20UL << WWDG_CFR_W_Pos)          /*!< 0x00000020 */
#define WWDG_CFR_W_6                        (0x40UL << WWDG_CFR_W_Pos)          /*!< 0x00000040 */

/* Legacy defines */
#define  WWDG_CFR_W0 WWDG_CFR_W_0
#define  WWDG_CFR_W1 WWDG_CFR_W_1
#define  WWDG_CFR_W2 WWDG_CFR_W_2
#define  WWDG_CFR_W3 WWDG_CFR_W_3
#define  WWDG_CFR_W4 WWDG_CFR_W_4
#define  WWDG_CFR_W5 WWDG_CFR_W_5
#define  WWDG_CFR_W6 WWDG_CFR_W_6

#define WWDG_CFR_WDGTB_Pos                  (7U)
#define WWDG_CFR_WDGTB_Msk                  (0x3UL << WWDG_CFR_WDGTB_Pos)       /*!< 0x00000180 */
#define WWDG_CFR_WDGTB                      WWDG_CFR_WDGTB_Msk                 /*!< WDGTB[1:0] bits (Timer Base) */
#define WWDG_CFR_WDGTB_0                    (0x1UL << WWDG_CFR_WDGTB_Pos)       /*!< 0x00000080 */
#define WWDG_CFR_WDGTB_1                    (0x2UL << WWDG_CFR_WDGTB_Pos)       /*!< 0x00000100 */

/* Legacy defines */
#define  WWDG_CFR_WDGTB0 WWDG_CFR_WDGTB_0
#define  WWDG_CFR_WDGTB1 WWDG_CFR_WDGTB_1

#define WWDG_CFR_EWI_Pos                    (9U)
#define WWDG_CFR_EWI_Msk                    (0x1UL << WWDG_CFR_EWI_Pos)         /*!< 0x00000200 */
#define WWDG_CFR_EWI                        WWDG_CFR_EWI_Msk                   /*!< Early Wakeup Interrupt */

/*******************  Bit definition for WWDG_SR register  ********************/
#define WWDG_SR_EWIF_Pos                    (0U)
#define WWDG_SR_EWIF_Msk                    (0x1UL << WWDG_SR_EWIF_Pos)         /*!< 0x00000001 */
#define WWDG_SR_EWIF                        WWDG_SR_EWIF_Msk                   /*!< Early Wakeup Interrupt Flag */

/*******************  Bit definition for RCC_CSR register  ********************/
#define RCC_CSR_LSION_Pos                    (0U)
#define RCC_CSR_LSION_Msk                    (0x1UL << RCC_CSR_LSION_Pos)       /*!< 0x00000001 */
#define RCC_CSR_LSION                        RCC_CSR_LSION_Msk                 /*!< Internal Low Speed oscillator enable */

#define RCC_CSR_LSIRDY_Pos                   (1U)
#define RCC_CSR_LSIRDY_Msk                   (0x1UL << RCC_CSR_LSIRDY_Pos)      /*!< 0x00000002 */
#define RCC_CSR_LSIRDY                       RCC_CSR_LSIRDY_Msk                /*!< Internal Low Speed oscillator Ready */

/* Private constants ---------------------------------------------------------*/
/** @defgroup IWDG_LL_Private_Constants IWDG Private Constants
  * @{
  */

#define LL_IWDG_KEY_RELOAD                 0x0000AAAAU               /*!< IWDG Reload Counter Enable   */
#define LL_IWDG_KEY_ENABLE                 0x0000CCCCU               /*!< IWDG Peripheral Enable       */
#define LL_IWDG_KEY_WR_ACCESS_ENABLE       0x00005555U               /*!< IWDG KR Write Access Enable  */
#define LL_IWDG_KEY_WR_ACCESS_DISABLE      0x00000000U               /*!< IWDG KR Write Access Disable */

/**
  * @brief Reset and Clock Control
  */

typedef struct
{
  __IO uint32_t CR;
  __IO uint32_t CFGR;
  __IO uint32_t CIR;
  __IO uint32_t APB2RSTR;
  __IO uint32_t APB1RSTR;
  __IO uint32_t AHBENR;
  __IO uint32_t APB2ENR;
  __IO uint32_t APB1ENR;
  __IO uint32_t BDCR;
  __IO uint32_t CSR;
} RCC_TypeDef;

/**
  * @brief Independent WATCHDOG
  */

typedef struct
{
  __IO uint32_t KR;           /*!< Key register,                                Address offset: 0x00 */
  __IO uint32_t PR;           /*!< Prescaler register,                          Address offset: 0x04 */
  __IO uint32_t RLR;          /*!< Reload register,                             Address offset: 0x08 */
  __IO uint32_t SR;           /*!< Status register,                             Address offset: 0x0C */
} IWDG_TypeDef;

/** @addtogroup Peripheral_memory_map
  * @{
  */
#define PERIPH_BASE           0x40000000UL /*!< Peripheral base address in the alias region */

/*!< Peripheral memory map */
#define APB1PERIPH_BASE       PERIPH_BASE
#define AHBPERIPH_BASE        (PERIPH_BASE + 0x00020000UL)

#define RCC_BASE              (AHBPERIPH_BASE + 0x00001000UL)
#define RCC                 ((RCC_TypeDef *)RCC_BASE)

#define IWDG_BASE             (APB1PERIPH_BASE + 0x00003000UL)
#define IWDG                ((IWDG_TypeDef *)IWDG_BASE)

/** @defgroup IWDG_LL_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_IWDG_ReadReg function
  * @{
  */
#define LL_IWDG_SR_PVU                     IWDG_SR_PVU                           /*!< Watchdog prescaler value update */
#define LL_IWDG_SR_RVU                     IWDG_SR_RVU                           /*!< Watchdog counter reload value update */

/** @defgroup IWDG_LL_EC_PRESCALER  Prescaler Divider
  * @{
  */
#define LL_IWDG_PRESCALER_4                0x00000000U                           /*!< Divider by 4   */
#define LL_IWDG_PRESCALER_8                (IWDG_PR_PR_0)                        /*!< Divider by 8   */
#define LL_IWDG_PRESCALER_16               (IWDG_PR_PR_1)                        /*!< Divider by 16  */
#define LL_IWDG_PRESCALER_32               (IWDG_PR_PR_1 | IWDG_PR_PR_0)         /*!< Divider by 32  */
#define LL_IWDG_PRESCALER_64               (IWDG_PR_PR_2)                        /*!< Divider by 64  */
#define LL_IWDG_PRESCALER_128              (IWDG_PR_PR_2 | IWDG_PR_PR_0)         /*!< Divider by 128 */
#define LL_IWDG_PRESCALER_256              (IWDG_PR_PR_2 | IWDG_PR_PR_1)         /*!< Divider by 256 */

/** @addtogroup Exported_macros
  * @{
  */
#define SET_BIT(REG, BIT)     ((REG) |= (BIT))

#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))

#define READ_BIT(REG, BIT)    ((REG) & (BIT))

#define CLEAR_REG(REG)        ((REG) = (0x0))

#define WRITE_REG(REG, VAL)   ((REG) = (VAL))

#define READ_REG(REG)         ((REG))

#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

/**
  * @brief  Enable LSI Oscillator
  * @rmtoll CSR          LSION         LL_RCC_LSI_Enable
  * @retval None
  */
void LL_RCC_LSI_Enable(void)
{
  SET_BIT(RCC->CSR, RCC_CSR_LSION);
}

/**
  * @brief  Check if LSI is Ready
  * @rmtoll CSR          LSIRDY        LL_RCC_LSI_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_LSI_IsReady(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_LSIRDY) == (RCC_CSR_LSIRDY));
}

/**
  * @brief  Check if RCC flag Independent Watchdog reset is set or not.
  * @rmtoll CSR          IWDGRSTF      LL_RCC_IsActiveFlag_IWDGRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_IWDGRST(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_IWDGRSTF) == (RCC_CSR_IWDGRSTF));
}

/**
  * @brief  Set RMVF bit to clear the reset flags.
  * @rmtoll CSR          RMVF          LL_RCC_ClearResetFlags
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearResetFlags(void)
{
  SET_BIT(RCC->CSR, RCC_CSR_RMVF);
}

/**
  * @brief  Start the Independent Watchdog
  * @note   Except if the hardware watchdog option is selected
  * @rmtoll KR           KEY           LL_IWDG_Enable
  * @param  IWDGx IWDG Instance
  * @retval None
  */
__STATIC_INLINE void LL_IWDG_Enable(IWDG_TypeDef *IWDGx)
{
  WRITE_REG(IWDG->KR, LL_IWDG_KEY_ENABLE);
}

/**
  * @brief  Select the prescaler of the IWDG
  * @rmtoll PR           PR            LL_IWDG_SetPrescaler
  * @param  IWDGx IWDG Instance
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_IWDG_PRESCALER_4
  *         @arg @ref LL_IWDG_PRESCALER_8
  *         @arg @ref LL_IWDG_PRESCALER_16
  *         @arg @ref LL_IWDG_PRESCALER_32
  *         @arg @ref LL_IWDG_PRESCALER_64
  *         @arg @ref LL_IWDG_PRESCALER_128
  *         @arg @ref LL_IWDG_PRESCALER_256
  * @retval None
  */
__STATIC_INLINE void LL_IWDG_SetPrescaler(IWDG_TypeDef *IWDGx, uint32_t Prescaler)
{
  WRITE_REG(IWDGx->PR, IWDG_PR_PR & Prescaler);
}

/**
  * @brief  Get the selected prescaler of the IWDG
  * @rmtoll PR           PR            LL_IWDG_GetPrescaler
  * @param  IWDGx IWDG Instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_IWDG_PRESCALER_4
  *         @arg @ref LL_IWDG_PRESCALER_8
  *         @arg @ref LL_IWDG_PRESCALER_16
  *         @arg @ref LL_IWDG_PRESCALER_32
  *         @arg @ref LL_IWDG_PRESCALER_64
  *         @arg @ref LL_IWDG_PRESCALER_128
  *         @arg @ref LL_IWDG_PRESCALER_256
  */
__STATIC_INLINE uint32_t LL_IWDG_GetPrescaler(IWDG_TypeDef *IWDGx)
{
  return (uint32_t)(READ_REG(IWDGx->PR));
}

/**
  * @brief  Check if flag Reload Value Update is set or not
  * @rmtoll SR           RVU           LL_IWDG_IsActiveFlag_RVU
  * @param  IWDGx IWDG Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_IWDG_IsActiveFlag_RVU(IWDG_TypeDef *IWDGx)
{
  return (READ_BIT(IWDGx->SR, IWDG_SR_RVU) == (IWDG_SR_RVU));
}

/**
  * @brief  Reloads IWDG counter with value defined in the reload register
  * @rmtoll KR           KEY           LL_IWDG_ReloadCounter
  * @param  IWDGx IWDG Instance
  * @retval None
  */
__STATIC_INLINE void LL_IWDG_ReloadCounter(IWDG_TypeDef *IWDGx)
{
  WRITE_REG(IWDG->KR, LL_IWDG_KEY_RELOAD);
}

/**
  * @brief  Enable write access to IWDG_PR, IWDG_RLR and IWDG_WINR registers
  * @rmtoll KR           KEY           LL_IWDG_EnableWriteAccess
  * @param  IWDGx IWDG Instance
  * @retval None
  */
__STATIC_INLINE void LL_IWDG_EnableWriteAccess(IWDG_TypeDef *IWDGx)
{
  WRITE_REG(IWDG->KR, LL_IWDG_KEY_WR_ACCESS_ENABLE);
}

/**
  * @brief  Specify the IWDG down-counter reload value
  * @rmtoll RLR          RL            LL_IWDG_SetReloadCounter
  * @param  IWDGx IWDG Instance
  * @param  Counter Value between Min_Data=0 and Max_Data=0x0FFF
  * @retval None
  */
__STATIC_INLINE void LL_IWDG_SetReloadCounter(IWDG_TypeDef *IWDGx, uint32_t Counter)
{
  WRITE_REG(IWDGx->RLR, IWDG_RLR_RL & Counter);
}

/**
  * @brief  Get the specified IWDG down-counter reload value
  * @rmtoll RLR          RL            LL_IWDG_GetReloadCounter
  * @param  IWDGx IWDG Instance
  * @retval Value between Min_Data=0 and Max_Data=0x0FFF
  */
__STATIC_INLINE uint32_t LL_IWDG_GetReloadCounter(IWDG_TypeDef *IWDGx)
{
  return (uint32_t)(READ_REG(IWDGx->RLR));
}

/**
  * @brief  Check if all flags Prescaler, Reload & Window Value Update are reset or not
  * @rmtoll SR           PVU           LL_IWDG_IsReady\n
  *         SR           RVU           LL_IWDG_IsReady
  * @param  IWDGx IWDG Instance
  * @retval State of bits (1 or 0).
  */
__STATIC_INLINE uint32_t LL_IWDG_IsReady(IWDG_TypeDef *IWDGx)
{
  return (READ_BIT(IWDGx->SR, IWDG_SR_PVU | IWDG_SR_RVU) == 0U);
}

/**
  * @brief  Check if flag Prescaler Value Update is set or not
  * @rmtoll SR           PVU           LL_IWDG_IsActiveFlag_PVU
  * @param  IWDGx IWDG Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_IWDG_IsActiveFlag_PVU(IWDG_TypeDef *IWDGx)
{
  return (READ_BIT(IWDGx->SR, IWDG_SR_PVU) == (IWDG_SR_PVU));
}

//---------------------------------------------------------------------------------------

// Minimal timeout in microseconds
#define IWDG_TIMEOUT_MIN    ((4*1000000)/LSI_VALUE)
// Maximal timeout in microseconds
#define IWDG_TIMEOUT_MAX    (((256*1000000)/LSI_VALUE)*IWDG_RLR_RL)

#define IS_IWDG_TIMEOUT(X)  (((X) >= IWDG_TIMEOUT_MIN) &&\
                             ((X) <= IWDG_TIMEOUT_MAX))

class IWatchdogClass {

  public:
    void begin(uint32_t timeout, uint32_t window = IWDG_TIMEOUT_MAX);
    void set(uint32_t timeout, uint32_t window = IWDG_TIMEOUT_MAX);
    void get(uint32_t *timeout, uint32_t *window = NULL);
    void reload(void);
    bool isEnabled(void)
    {
      return _enabled;
    };
    bool isReset(bool clear = false);
    void clearReset(void);

  private:
    static bool _enabled;
};

extern IWatchdogClass IWatchdog;

#endif
