#ifndef _RTC_H
#define _RTC_H

#include "types.h"

#define RTC_REG_B 0x8B
#define RTC_REG_A 0x8A
#define RTC_CMD_PORT 0x70
#define RTC_DATA_PORT 0x71
#define BIT6_ON 0x40
#define RTC_PIC_PORT 0x8
#define RTC_FREQ_CONST 0x06

#define RTC_MAX_FREQ 1024

// Initialize RTC device
void rtc_init(void);

// Open the RTC device
int32_t rtc_open(const int8_t* filename);

// Close the RTC device
int32_t rtc_close(int32_t fd);

// Block until RTC interrupt arrives
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

// Modify RTC interrupt rate
int32_t rtc_write(int32_t fd, void* buf, int32_t nbytes);

void default_rtc_handler(void);

#endif

