#pragma once

#include <signal.h>

extern volatile sig_atomic_t interrupted;

void signals_init(void);
void signals_reset(void);