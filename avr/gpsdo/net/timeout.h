/* vim: set sw=8 ts=8 si et: */
#ifndef F_CPU
#define F_CPU 12500000UL  // 12.5 MHz
#else 
#warning "F_CPU is already defined"
#endif

#ifndef ALIBC_OLD
#include <util/delay.h>
#else
#include <avr/delay.h>
#endif

