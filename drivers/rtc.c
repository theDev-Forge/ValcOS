#include "rtc.h"
#include "idt.h"

// Helper: Read RTC register
static uint8_t rtc_read_register(uint8_t reg) {
    outb(RTC_ADDRESS, reg);
    return inb(RTC_DATA);
}

// Helper: Check if RTC is updating
static int rtc_is_updating(void) {
    outb(RTC_ADDRESS, RTC_STATUS_A);
    return (inb(RTC_DATA) & 0x80);
}

// Helper: Convert BCD to binary
static uint8_t bcd_to_binary(uint8_t bcd) {
    return ((bcd / 16) * 10) + (bcd & 0x0F);
}

void rtc_init(void) {
    // Disable NMI and read status register B
    outb(RTC_ADDRESS, RTC_STATUS_B);
    uint8_t status_b = inb(RTC_DATA);
    
    // Check if RTC is in BCD mode (bit 2 = 0 means BCD)
    // Most systems use BCD mode
    
    // Re-enable NMI (we'll handle it per-read)
}

void rtc_read_time(uint8_t *hour, uint8_t *minute, uint8_t *second) {
    // Wait until RTC is not updating
    while (rtc_is_updating());
    
    uint8_t sec = rtc_read_register(RTC_SECONDS);
    uint8_t min = rtc_read_register(RTC_MINUTES);
    uint8_t hr = rtc_read_register(RTC_HOURS);
    
    // Check if BCD mode (most common)
    outb(RTC_ADDRESS, RTC_STATUS_B);
    uint8_t status_b = inb(RTC_DATA);
    
    if (!(status_b & 0x04)) {
        // BCD mode - convert to binary
        sec = bcd_to_binary(sec);
        min = bcd_to_binary(min);
        hr = bcd_to_binary(hr);
    }
    
    // Handle 12-hour format if needed
    if (!(status_b & 0x02) && (hr & 0x80)) {
        hr = ((hr & 0x7F) + 12) % 24;
    }
    
    if (hour) *hour = hr;
    if (minute) *minute = min;
    if (second) *second = sec;
}

void rtc_read_date(uint16_t *year, uint8_t *month, uint8_t *day) {
    // Wait until RTC is not updating
    while (rtc_is_updating());
    
    uint8_t d = rtc_read_register(RTC_DAY);
    uint8_t m = rtc_read_register(RTC_MONTH);
    uint8_t y = rtc_read_register(RTC_YEAR);
    
    // Check if BCD mode
    outb(RTC_ADDRESS, RTC_STATUS_B);
    uint8_t status_b = inb(RTC_DATA);
    
    if (!(status_b & 0x04)) {
        // BCD mode - convert to binary
        d = bcd_to_binary(d);
        m = bcd_to_binary(m);
        y = bcd_to_binary(y);
    }
    
    // RTC year is 2-digit, assume 2000+
    uint16_t full_year = 2000 + y;
    
    if (year) *year = full_year;
    if (month) *month = m;
    if (day) *day = d;
}

uint32_t rtc_get_timestamp(void) {
    uint8_t hour, minute, second;
    uint16_t year;
    uint8_t month, day;
    
    rtc_read_time(&hour, &minute, &second);
    rtc_read_date(&year, &month, &day);
    
    // Simple timestamp calculation (seconds since 2000-01-01)
    // This is a simplified version - doesn't account for leap years properly
    uint32_t days_since_2000 = (year - 2000) * 365;
    days_since_2000 += (month - 1) * 30; // Approximate
    days_since_2000 += day;
    
    uint32_t timestamp = days_since_2000 * 86400; // Days to seconds
    timestamp += hour * 3600;
    timestamp += minute * 60;
    timestamp += second;
    
    return timestamp;
}
