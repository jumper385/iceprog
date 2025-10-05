#ifndef ICEPROG_FN_H
#define ICEPROG_FN_H

#include <stdbool.h>
#include "mpsse.h"

void set_cs_creset(int cs_b, int creset_b);
bool get_cdone(void);
void flash_release_reset();
void flash_chip_select();
void flash_chip_deselect();
void sram_reset();
void sram_chip_select();
void flash_read_id();
void flash_reset();
void flash_power_up();
void flash_power_down();
uint8_t flash_read_status();
void flash_write_enable();
void flash_bulk_erase();
void flash_4kB_sector_erase(int addr);
void flash_32kB_sector_erase(int addr);
void flash_64kB_sector_erase(int addr);
void flash_prog(int addr, uint8_t *data, int n);
void flash_read(int addr, uint8_t *data, int n);
void flash_wait();
void flash_disable_protection();
void flash_enable_quad();
void help(const char *progname);

#endif // ICEPROG_FN_H