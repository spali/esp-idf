// Copyright 2015-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "soc/soc.h"
#include "soc/cpu.h"
#include "soc/rtc_periph.h"
#include "soc/rtc_cntl_reg.h"
#include "esp32/rom/ets_sys.h"
#include "esp_private/system_internal.h"
#include "driver/rtc_cntl.h"
#include "freertos/FreeRTOS.h"
#include "esp_attr.h"

#if CONFIG_ESP_SYSTEM_BROWNOUT_INTR
#include "esp_intr_alloc.h"
#include "bootloader_flash.h"
#include "esp_private/spi_flash_os.h"
#endif

#ifdef CONFIG_ESP32_BROWNOUT_DET_LVL
#define BROWNOUT_DET_LVL CONFIG_ESP32_BROWNOUT_DET_LVL
#else
#define BROWNOUT_DET_LVL 0
#endif

IRAM_ATTR static void rtc_brownout_isr_handler(void *arg)
{
    REG_WRITE(RTC_CNTL_INT_CLR_REG, RTC_CNTL_BROWN_OUT_INT_CLR);
    esp_cpu_stall(!xPortGetCoreID());
    esp_reset_reason_set_hint(ESP_RST_BROWNOUT);
#if CONFIG_SPI_FLASH_BROWNOUT_RESET
    if (spi_flash_brownout_need_reset()) {
        if (spi_flash_brownout_check_erase()) {
            bootloader_flash_reset_chip();
        }
    }
#endif
    ets_printf(DRAM_STR("\r\nBrownout detector was triggered\r\n\r\n"));
    esp_restart_noos();
}

void esp_brownout_init(void)
{
#if CONFIG_ESP_SYSTEM_BROWNOUT_INTR
    REG_WRITE(RTC_CNTL_BROWN_OUT_REG,
            RTC_CNTL_BROWN_OUT_ENA
            | RTC_CNTL_BROWN_OUT_CLOSE_FLASH_ENA_M
            | RTC_CNTL_BROWN_OUT_PD_RF_ENA_M
            | (2 << RTC_CNTL_BROWN_OUT_RST_WAIT_S)
            | (BROWNOUT_DET_LVL << RTC_CNTL_DBROWN_OUT_THRES_S));
    REG_WRITE(RTC_CNTL_INT_CLR_REG, RTC_CNTL_BROWN_OUT_INT_CLR);
#else
    REG_WRITE(RTC_CNTL_BROWN_OUT_REG,
            RTC_CNTL_BROWN_OUT_ENA
            | RTC_CNTL_BROWN_OUT_RST_ENA
            | RTC_CNTL_BROWN_OUT_PD_RF_ENA_M
            | (2 << RTC_CNTL_BROWN_OUT_RST_WAIT_S)
            | (BROWNOUT_DET_LVL << RTC_CNTL_DBROWN_OUT_THRES_S));
#endif
    ESP_ERROR_CHECK(rtc_isr_register(rtc_brownout_isr_handler, NULL, RTC_CNTL_BROWN_OUT_INT_ENA_M, RTC_INTR_FLAG_IRAM));
    REG_SET_BIT(RTC_CNTL_INT_ENA_REG, RTC_CNTL_BROWN_OUT_INT_ENA_M);

}

void esp_brownout_disable(void)
{
    REG_CLR_BIT(RTC_CNTL_INT_ENA_REG, RTC_CNTL_BROWN_OUT_INT_ENA_M);
    rtc_isr_deregister(rtc_brownout_isr_handler, NULL);
    REG_WRITE(RTC_CNTL_BROWN_OUT_REG, 0);
}
