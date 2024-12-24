/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
 * NOTICE
 * The ll is not public api, don't use in application code.
 * See readme.md in soc/include/hal/readme.md
 ******************************************************************************/

// The Lowlevel layer for SPI Flash

#pragma once

#include "spimem_flash_ll.h"

//TODO: [ESP32H21] IDF-11609, inherit from h2, need to add gpspi functions

#ifdef __cplusplus
extern "C" {
#endif

#define spi_flash_ll_calculate_clock_reg(host_id, clock_div) (((host_id)<=SPI1_HOST) ? spimem_flash_ll_calculate_clock_reg(clock_div) \
                                            : 0)

#define spi_flash_ll_get_source_clock_freq_mhz(host_id)  (((host_id)<=SPI1_HOST) ? spimem_flash_ll_get_source_freq_mhz() : 0)

#define spi_flash_ll_get_hw(host_id)  (((host_id)<=SPI1_HOST ? (spi_dev_t*) spimem_flash_ll_get_hw(host_id) \
                                      : 0))

#define spi_flash_ll_hw_get_id(dev) ({int dev_id = spimem_flash_ll_hw_get_id(dev); \
                                     if (dev_id < 0) {\
                                        dev_id = 0;\
                                     }\
                                     dev_id; \
                                    })
// Since ESP32-H2, WB_mode is available, we extend 8 bits to occupy `Continuous Read Mode` bits.
#define SPI_FLASH_LL_CONTINUOUS_MODE_BIT_NUMS  (8)

typedef union  {
    spimem_flash_ll_clock_reg_t spimem;
} spi_flash_ll_clock_reg_t;

#define SPIMEM_LL_APB SPIMEM1
#define SPIMEM_LL_CACHE SPIMEM0

#ifdef GPSPI_BUILD
#define spi_flash_ll_reset(dev)                              gpspi_flash_ll_reset((spi_dev_t*)dev)
#define spi_flash_ll_cmd_is_done(dev)                        gpspi_flash_ll_cmd_is_done((spi_dev_t*)dev)
#define spi_flash_ll_get_buffer_data(dev, buffer, read_len)  gpspi_flash_ll_get_buffer_data((spi_dev_t*)dev, buffer, read_len)
#define spi_flash_ll_set_buffer_data(dev, buffer, len)       gpspi_flash_ll_set_buffer_data((spi_dev_t*)dev, buffer, len)
#define spi_flash_ll_user_start(dev, pe_ops)                 gpspi_flash_ll_user_start((spi_dev_t*)dev, pe_ops)
#define spi_flash_ll_host_idle(dev)                          gpspi_flash_ll_host_idle((spi_dev_t*)dev)
#define spi_flash_ll_read_phase(dev)                         gpspi_flash_ll_read_phase((spi_dev_t*)dev)
#define spi_flash_ll_set_cs_pin(dev, pin)                    gpspi_flash_ll_set_cs_pin((spi_dev_t*)dev, pin)
#define spi_flash_ll_set_read_mode(dev, read_mode)           gpspi_flash_ll_set_read_mode((spi_dev_t*)dev, read_mode)
#define spi_flash_ll_set_clock(dev, clk)                     gpspi_flash_ll_set_clock((spi_dev_t*)dev, (gpspi_flash_ll_clock_reg_t*)clk)
#define spi_flash_ll_set_miso_bitlen(dev, bitlen)            gpspi_flash_ll_set_miso_bitlen((spi_dev_t*)dev, bitlen)
#define spi_flash_ll_set_mosi_bitlen(dev, bitlen)            gpspi_flash_ll_set_mosi_bitlen((spi_dev_t*)dev, bitlen)
#define spi_flash_ll_set_command(dev, cmd, bitlen)           gpspi_flash_ll_set_command((spi_dev_t*)dev, cmd, bitlen)
#define spi_flash_ll_set_addr_bitlen(dev, bitlen)            gpspi_flash_ll_set_addr_bitlen((spi_dev_t*)dev, bitlen)
#define spi_flash_ll_get_addr_bitlen(dev)                    gpspi_flash_ll_get_addr_bitlen((spi_dev_t*)dev)
#define spi_flash_ll_set_address(dev, addr)                  gpspi_flash_ll_set_address((spi_dev_t*)dev, addr)
#define spi_flash_ll_set_usr_address(dev, addr, bitlen)      gpspi_flash_ll_set_usr_address((spi_dev_t*)dev, addr, bitlen)
#define spi_flash_ll_set_dummy(dev, dummy)                   gpspi_flash_ll_set_dummy((spi_dev_t*)dev, dummy)
#define spi_flash_ll_set_hold(dev, hold_n)                   gpspi_flash_ll_set_hold((spi_dev_t*)dev, hold_n)
#define spi_flash_ll_set_cs_setup(dev, cs_setup_time)        gpspi_flash_ll_set_cs_setup((spi_dev_t*)dev, cs_setup_time)
#define spi_flash_ll_set_extra_address(dev, extra_addr)      { /* Not supported on gpspi on ESP32-H2*/ }
#else
#define spi_flash_ll_reset(dev)                              spimem_flash_ll_reset((spi_mem_dev_t*)dev)
#define spi_flash_ll_cmd_is_done(dev)                        spimem_flash_ll_cmd_is_done((spi_mem_dev_t*)dev)
#define spi_flash_ll_erase_chip(dev)                         spimem_flash_ll_erase_chip((spi_mem_dev_t*)dev)
#define spi_flash_ll_erase_sector(dev)                       spimem_flash_ll_erase_sector((spi_mem_dev_t*)dev)
#define spi_flash_ll_erase_block(dev)                        spimem_flash_ll_erase_block((spi_mem_dev_t*)dev)
#define spi_flash_ll_set_write_protect(dev, wp)              spimem_flash_ll_set_write_protect((spi_mem_dev_t*)dev, wp)
#define spi_flash_ll_get_buffer_data(dev, buffer, read_len)  spimem_flash_ll_get_buffer_data((spi_mem_dev_t*)dev, buffer, read_len)
#define spi_flash_ll_set_buffer_data(dev, buffer, len)       spimem_flash_ll_set_buffer_data((spi_mem_dev_t*)dev, buffer, len)
#define spi_flash_ll_program_page(dev, buffer, len)          spimem_flash_ll_program_page((spi_mem_dev_t*)dev, buffer, len)
#define spi_flash_ll_user_start(dev, pe_ops)                 spimem_flash_ll_user_start((spi_mem_dev_t*)dev, pe_ops)
#define spi_flash_ll_host_idle(dev)                          spimem_flash_ll_host_idle((spi_mem_dev_t*)dev)
#define spi_flash_ll_read_phase(dev)                         spimem_flash_ll_read_phase((spi_mem_dev_t*)dev)
#define spi_flash_ll_set_cs_pin(dev, pin)                    spimem_flash_ll_set_cs_pin((spi_mem_dev_t*)dev, pin)
#define spi_flash_ll_set_read_mode(dev, read_mode)           spimem_flash_ll_set_read_mode((spi_mem_dev_t*)dev, read_mode)
#define spi_flash_ll_set_clock(dev, clk)                     spimem_flash_ll_set_clock((spi_mem_dev_t*)dev, (spimem_flash_ll_clock_reg_t*)clk)
#define spi_flash_ll_set_miso_bitlen(dev, bitlen)            spimem_flash_ll_set_miso_bitlen((spi_mem_dev_t*)dev, bitlen)
#define spi_flash_ll_set_mosi_bitlen(dev, bitlen)            spimem_flash_ll_set_mosi_bitlen((spi_mem_dev_t*)dev, bitlen)
#define spi_flash_ll_set_command(dev, cmd, bitlen)           spimem_flash_ll_set_command((spi_mem_dev_t*)dev, cmd, bitlen)
#define spi_flash_ll_set_addr_bitlen(dev, bitlen)            spimem_flash_ll_set_addr_bitlen((spi_mem_dev_t*)dev, bitlen)
#define spi_flash_ll_get_addr_bitlen(dev)                    spimem_flash_ll_get_addr_bitlen((spi_mem_dev_t*) dev)
#define spi_flash_ll_set_address(dev, addr)                  spimem_flash_ll_set_address((spi_mem_dev_t*)dev, addr)
#define spi_flash_ll_set_usr_address(dev, addr, bitlen)      spimem_flash_ll_set_usr_address((spi_mem_dev_t*)dev, addr, bitlen)
#define spi_flash_ll_set_dummy(dev, dummy)                   spimem_flash_ll_set_dummy((spi_mem_dev_t*)dev, dummy)
#define spi_flash_ll_set_hold(dev, hold_n)                   spimem_flash_ll_set_hold((spi_mem_dev_t*)dev, hold_n)
#define spi_flash_ll_set_cs_setup(dev, cs_setup_time)        spimem_flash_ll_set_cs_setup((spi_mem_dev_t*)dev, cs_setup_time)
#define spi_flash_ll_set_extra_address(dev, extra_addr)      spimem_flash_ll_set_extra_address((spi_mem_dev_t*)dev, extra_addr)
#define spi_flash_ll_get_ctrl_val(dev)                       spimem_flash_ll_get_ctrl_val((spi_mem_dev_t*)dev)
#define spi_flash_ll_sync_reset()                            spimem_flash_ll_sync_reset()
#define spi_flash_ll_set_common_command_register_info(dev, ctrl_reg, user_reg, user1_reg, user2_reg)        spimem_flash_ll_set_common_command_register_info((spi_mem_dev_t*)dev, ctrl_reg, user_reg, user1_reg, user2_reg)
#define spi_flash_ll_get_common_command_register_info(dev, ctrl_reg, user_reg, user1_reg, user2_reg)        spimem_flash_ll_get_common_command_register_info((spi_mem_dev_t*)dev, ctrl_reg, user_reg, user1_reg, user2_reg)


#endif

#ifdef __cplusplus
}
#endif