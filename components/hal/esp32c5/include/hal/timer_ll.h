/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Attention: Timer Group has 3 independent functions: General Purpose Timer, Watchdog Timer and Clock calibration.
//            This Low Level driver only serve the General Purpose Timer function.

#pragma once

#include <stdbool.h>
#include "sdkconfig.h"  // TODO: remove
#include "hal/assert.h"
#include "hal/misc.h"
#include "hal/timer_types.h"
#include "soc/timer_group_struct.h"
#include "soc/pcr_struct.h"
// TODO: [ESP32C5] IDF-8693
// #include "soc/soc_etm_source.h"

#ifdef __cplusplus
extern "C" {
#endif

// Get timer group register base address with giving group number
#define TIMER_LL_GET_HW(group_id) ((group_id == 0) ? (&TIMERG0) : (&TIMERG1))
#define TIMER_LL_EVENT_ALARM(timer_id) (1 << (timer_id))


/**
 * @brief Enable the bus clock for timer group module
 *
 * @param group_id Group ID
 * @param enable true to enable, false to disable
 */
static inline void timer_ll_enable_bus_clock(int group_id, bool enable)
{
    if (group_id == 0) {
        PCR.timergroup0_conf.tg0_clk_en = enable;
    } else {
        PCR.timergroup1_conf.tg1_clk_en = enable;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_RC_ATOMIC_ENV variable in advance
#define timer_ll_enable_bus_clock(...) (void)__DECLARE_RCC_RC_ATOMIC_ENV; timer_ll_enable_bus_clock(__VA_ARGS__)

/**
 * @brief Reset the timer group module
 *
 * @note  After reset the register, the "flash boot protection" will be enabled again.
 *        FLash boot protection is not used anymore after system boot up.
 *        This function will disable it by default in order to prevent the system from being reset unexpectedly.
 *
 * @param group_id Group ID
 */
static inline void timer_ll_reset_register(int group_id)
{
    if (group_id == 0) {
        PCR.timergroup0_conf.tg0_rst_en = 1;
        PCR.timergroup0_conf.tg0_rst_en = 0;
        TIMERG0.wdtconfig0.wdt_flashboot_mod_en = 0;
    } else {
        PCR.timergroup1_conf.tg1_rst_en = 1;
        PCR.timergroup1_conf.tg1_rst_en = 0;
        TIMERG1.wdtconfig0.wdt_flashboot_mod_en = 0;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_RC_ATOMIC_ENV variable in advance
#define timer_ll_reset_register(...) (void)__DECLARE_RCC_RC_ATOMIC_ENV; timer_ll_reset_register(__VA_ARGS__)

/**
 * @brief Set clock source for timer
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 * @param clk_src Clock source
 */
static inline void timer_ll_set_clock_source(timg_dev_t *hw, uint32_t timer_num, gptimer_clock_source_t clk_src)
{
    (void)timer_num; // only one timer in each group
    uint8_t clk_id = 0;
    switch (clk_src) {
    case GPTIMER_CLK_SRC_XTAL:
        clk_id = 0;
        break;
    case GPTIMER_CLK_SRC_RC_FAST:
        clk_id = 1;
        break;
    case GPTIMER_CLK_SRC_PLL_F80M:
        clk_id = 2;
        break;
    default:
        HAL_ASSERT(false);
        break;
    }
    if (hw == &TIMERG0) {
        PCR.timergroup0_timer_clk_conf.tg0_timer_clk_sel = clk_id;
    } else {
        PCR.timergroup1_timer_clk_conf.tg1_timer_clk_sel = clk_id;
    }
}

/**
 * @brief Enable Timer Group (GPTimer) module clock
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer index in the group
 * @param en true to enable, false to disable
 */
static inline void timer_ll_enable_clock(timg_dev_t *hw, uint32_t timer_num, bool en)
{
    (void)timer_num; // only one timer in each group
    if (hw == &TIMERG0) {
        PCR.timergroup0_timer_clk_conf.tg0_timer_clk_en = en;
    } else {
        PCR.timergroup1_timer_clk_conf.tg1_timer_clk_en = en;
    }
}

/**
 * @brief Enable alarm event
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 * @param en True: enable alarm
 *           False: disable alarm
 */
__attribute__((always_inline))
static inline void timer_ll_enable_alarm(timg_dev_t *hw, uint32_t timer_num, bool en)
{
    (void)timer_num;
#if CONFIG_IDF_TARGET_ESP32C5_BETA3_VERSION
    hw->hw_timer[timer_num].config.tx_alarm_en = en;
#elif CONFIG_IDF_TARGET_ESP32C5_MP_VERSION
    abort();
#endif
}

/**
 * @brief Set clock prescale for timer
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 * @param divider Prescale value (0 and 1 are not valid)
 */
static inline void timer_ll_set_clock_prescale(timg_dev_t *hw, uint32_t timer_num, uint32_t divider)
{
    HAL_ASSERT(divider >= 2 && divider <= 65536);
    if (divider >= 65536) {
        divider = 0;
    }
#if CONFIG_IDF_TARGET_ESP32C5_BETA3_VERSION
    HAL_FORCE_MODIFY_U32_REG_FIELD(hw->hw_timer[timer_num].config, tx_divider, divider);
    hw->hw_timer[timer_num].config.tx_divcnt_rst = 1;
#elif CONFIG_IDF_TARGET_ESP32C5_MP_VERSION
    abort();
#endif
}

/**
 * @brief Enable auto-reload mode
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 * @param en True: enable auto reload mode
 *           False: disable auto reload mode
 */
__attribute__((always_inline))
static inline void timer_ll_enable_auto_reload(timg_dev_t *hw, uint32_t timer_num, bool en)
{
#if CONFIG_IDF_TARGET_ESP32C5_BETA3_VERSION
    hw->hw_timer[timer_num].config.tx_autoreload = en;
#elif CONFIG_IDF_TARGET_ESP32C5_MP_VERSION
    abort();
#endif
}

/**
 * @brief Set count direction
 *
 * @param hw Timer peripheral register base address
 * @param timer_num Timer number in the group
 * @param direction Count direction
 */
static inline void timer_ll_set_count_direction(timg_dev_t *hw, uint32_t timer_num, gptimer_count_direction_t direction)
{
#if CONFIG_IDF_TARGET_ESP32C5_BETA3_VERSION
    hw->hw_timer[timer_num].config.tx_increase = (direction == GPTIMER_COUNT_UP);
#elif CONFIG_IDF_TARGET_ESP32C5_MP_VERSION
    abort();
#endif
}

/**
 * @brief Enable timer, start couting
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 * @param en True: enable the counter
 *           False: disable the counter
 */
__attribute__((always_inline))
static inline void timer_ll_enable_counter(timg_dev_t *hw, uint32_t timer_num, bool en)
{
#if CONFIG_IDF_TARGET_ESP32C5_BETA3_VERSION
    hw->hw_timer[timer_num].config.tx_en = en;
#elif CONFIG_IDF_TARGET_ESP32C5_MP_VERSION
    abort();
#endif
}

/**
 * @brief Trigger software capture event
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 */
__attribute__((always_inline))
static inline void timer_ll_trigger_soft_capture(timg_dev_t *hw, uint32_t timer_num)
{
#if CONFIG_IDF_TARGET_ESP32C5_BETA3_VERSION
    hw->hw_timer[timer_num].update.tx_update = 1;
    // Timer register is in a different clock domain from Timer hardware logic
    // We need to wait for the update to take effect before fetching the count value
    while (hw->hw_timer[timer_num].update.tx_update) {
    }
#elif CONFIG_IDF_TARGET_ESP32C5_MP_VERSION
    abort();
#endif
}

/**
 * @brief Get counter value
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 *
 * @return counter value
 */
__attribute__((always_inline))
static inline uint64_t timer_ll_get_counter_value(timg_dev_t *hw, uint32_t timer_num)
{
#if CONFIG_IDF_TARGET_ESP32C5_BETA3_VERSION
    return ((uint64_t)hw->hw_timer[timer_num].hi.tx_hi << 32) | (hw->hw_timer[timer_num].lo.tx_lo);
#elif CONFIG_IDF_TARGET_ESP32C5_MP_VERSION
    abort();
    return 0;
#endif
}

/**
 * @brief Set alarm value
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 * @param alarm_value When counter reaches alarm value, alarm event will be triggered
 */
__attribute__((always_inline))
static inline void timer_ll_set_alarm_value(timg_dev_t *hw, uint32_t timer_num, uint64_t alarm_value)
{
#if CONFIG_IDF_TARGET_ESP32C5_BETA3_VERSION
    hw->hw_timer[timer_num].alarmhi.tx_alarm_hi = (uint32_t)(alarm_value >> 32);
    hw->hw_timer[timer_num].alarmlo.tx_alarm_lo = (uint32_t)alarm_value;
#elif CONFIG_IDF_TARGET_ESP32C5_MP_VERSION
    abort();
#endif
}

/**
 * @brief Set reload value
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 * @param reload_val Reload counter value
 */
__attribute__((always_inline))
static inline void timer_ll_set_reload_value(timg_dev_t *hw, uint32_t timer_num, uint64_t reload_val)
{
#if CONFIG_IDF_TARGET_ESP32C5_BETA3_VERSION
    hw->hw_timer[timer_num].loadhi.tx_load_hi = (uint32_t)(reload_val >> 32);
    hw->hw_timer[timer_num].loadlo.tx_load_lo = (uint32_t)reload_val;
#elif CONFIG_IDF_TARGET_ESP32C5_MP_VERSION
    abort();
#endif
}

/**
 * @brief Get reload value
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 * @return reload count value
 */
__attribute__((always_inline))
static inline uint64_t timer_ll_get_reload_value(timg_dev_t *hw, uint32_t timer_num)
{
#if CONFIG_IDF_TARGET_ESP32C5_BETA3_VERSION
    return ((uint64_t)hw->hw_timer[timer_num].loadhi.tx_load_hi << 32) | (hw->hw_timer[timer_num].loadlo.tx_load_lo);
#elif CONFIG_IDF_TARGET_ESP32C5_MP_VERSION
    abort();
    return 0;
#endif
}

/**
 * @brief Trigger software reload, value set by `timer_ll_set_reload_value()` will be reflected into counter immediately
 *
 * @param hw Timer Group register base address
 * @param timer_num Timer number in the group
 */
__attribute__((always_inline))
static inline void timer_ll_trigger_soft_reload(timg_dev_t *hw, uint32_t timer_num)
{
#if CONFIG_IDF_TARGET_ESP32C5_BETA3_VERSION
    hw->hw_timer[timer_num].load.tx_load = 1;
#elif CONFIG_IDF_TARGET_ESP32C5_MP_VERSION
    abort();
#endif
}

/**
 * @brief Enable ETM module
 *
 * @param hw Timer Group register base address
 * @param en True: enable ETM module, False: disable ETM module
 */
static inline void timer_ll_enable_etm(timg_dev_t *hw, bool en)
{
    hw->regclk.etm_en = en;
}

/**
 * @brief Enable timer interrupt by mask
 *
 * @param hw Timer Group register base address
 * @param mask Mask of interrupt events
 * @param en True: enable interrupt
 *           False: disable interrupt
 */
__attribute__((always_inline))
static inline void timer_ll_enable_intr(timg_dev_t *hw, uint32_t mask, bool en)
{
    if (en) {
        hw->int_ena_timers.val |= mask;
    } else {
        hw->int_ena_timers.val &= ~mask;
    }
}

/**
 * @brief Get interrupt status
 *
 * @param hw Timer Group register base address
 *
 * @return Interrupt status
 */
__attribute__((always_inline))
static inline uint32_t timer_ll_get_intr_status(timg_dev_t *hw)
{
    return hw->int_st_timers.val & 0x01;
}

/**
 * @brief Clear interrupt status by mask
 *
 * @param hw Timer Group register base address
 * @param mask Interrupt events mask
 */
__attribute__((always_inline))
static inline void timer_ll_clear_intr_status(timg_dev_t *hw, uint32_t mask)
{
    hw->int_clr_timers.val = mask;
}

/**
 * @brief Enable the register clock forever
 *
 * @param hw Timer Group register base address
 * @param en True: Enable the register clock forever
 *           False: Register clock is enabled only when register operation happens
 */
static inline void timer_ll_enable_register_clock_always_on(timg_dev_t *hw, bool en)
{
    hw->regclk.clk_en = en;
}

/**
 * @brief Get interrupt status register address
 *
 * @param hw Timer Group register base address
 *
 * @return Interrupt status register address
 */
static inline volatile void *timer_ll_get_intr_status_reg(timg_dev_t *hw)
{
    return &hw->int_st_timers;
}

#ifdef __cplusplus
}
#endif