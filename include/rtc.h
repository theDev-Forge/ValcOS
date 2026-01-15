#ifndef RTC_H
#define RTC_H

#include <stdint.h>

// RTC Ports
#define RTC_ADDRESS 0x70
#define RTC_DATA    0x71

// RTC Registers
#define RTC_SECONDS     0x00
#define RTC_MINUTES     0x02
#define RTC_HOURS       0x04
#define RTC_DAY         0x07
#define RTC_MONTH       0x08
#define RTC_YEAR        0x09
#define RTC_STATUS_A    0x0A
#define RTC_STATUS_B    0x0B

// Initialize RTC
void rtc_init(void);

// Read time components
void rtc_read_time(uint8_t *hour, uint8_t *minute, uint8_t *second);

// Read date components
void rtc_read_date(uint16_t *year, uint8_t *month, uint8_t *day);

// Get Unix-like timestamp (seconds since 2000-01-01)
uint32_t rtc_get_timestamp(void);

#endif
