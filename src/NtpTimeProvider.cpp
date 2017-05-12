//
//  NtpTimeProvider.cpp
//  esp-home-iot
//
//  Created by <author> on 10/05/2017.
//
//

#include "NtpTimeProvider.hpp"

TimeClient* NtpTimeProvider::timeClient = NULL;

NtpTimeProvider::NtpTimeProvider() : TimeProvider() {
    if (timeClient == NULL) {
        timeClient = new TimeClient(UTC_OFFSET);
        timeClient->updateTime();
    }
}
unsigned long NtpTimeProvider::getSecondsOfDay(){
    return timeClient->getCurrentEpochWithUtcOffset() % SECONDS_IN_A_DAY;
}

void NtpTimeProvider::updateTime() {
    timeClient->updateTime();
}
