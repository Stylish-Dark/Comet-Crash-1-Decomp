#pragma once
#include <stdint.h>

void comet_host_init(void);
void comet_host_shutdown(void);
void comet_host_install_hle_overrides(void);
void comet_host_pump(void);
uint32_t comet_host_width(void);
uint32_t comet_host_height(void);
int comet_host_vsync(void);
