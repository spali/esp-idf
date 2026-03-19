// Copyright 2015-2022 Espressif Systems (Shanghai) CO LTD
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void spi_flash_needs_reset_check(void);
void spi_flash_set_erasing_flag(bool status);
bool spi_flash_brownout_need_reset(void);
bool spi_flash_brownout_check_erase(void);

#ifdef __cplusplus
}
#endif
