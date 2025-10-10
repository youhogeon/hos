#include "RTC.h"
#include "../util/assembly.h"

/**
 * RTC 현재시간 조회
 */
void kReadRTCTime(BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond) {
    BYTE bData;

    outb(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
    bData = inb(RTC_CMOSDATA);
    *pbHour = RTC_BCDTOBINARY(bData);

    outb(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);
    bData = inb(RTC_CMOSDATA);
    *pbMinute = RTC_BCDTOBINARY(bData);

    outb(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);
    bData = inb(RTC_CMOSDATA);
    *pbSecond = RTC_BCDTOBINARY(bData);
}

/**
 * RTC 현재날짜 조회
 */
void kReadRTCDate(WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, BYTE* pbDayOfWeek) {
    BYTE bData;

    outb(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);
    bData = inb(RTC_CMOSDATA);
    *pwYear = RTC_BCDTOBINARY(bData) + 2000;

    outb(RTC_CMOSADDRESS, RTC_ADDRESS_MONTH);
    bData = inb(RTC_CMOSDATA);
    *pbMonth = RTC_BCDTOBINARY(bData);

    outb(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH);
    bData = inb(RTC_CMOSDATA);
    *pbDayOfMonth = RTC_BCDTOBINARY(bData);

    outb(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);
    bData = inb(RTC_CMOSDATA);
    *pbDayOfWeek = RTC_BCDTOBINARY(bData);
}

char* kConvertDayOfWeekToString(BYTE bDayOfWeek) {
    static char* vpcDayOfWeekString[8] =
        {"Error", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    if (bDayOfWeek >= 8) {
        return vpcDayOfWeekString[0];
    }

    return vpcDayOfWeekString[bDayOfWeek];
}