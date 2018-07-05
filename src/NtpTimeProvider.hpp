//
//  NtpTimeProvider.hpp
//  esp-home-iot
//
//  Created by <author> on 10/05/2017.
//
//

#ifndef NtpTimeProvider_hpp
#define NtpTimeProvider_hpp

#include <stdio.h>
#include <TimeClient.h>
#include "utils.h"
#include "config.h"

class NtpTimeProvider : public TimeProvider {
private:
    static TimeClient * timeClient;
public:
    NtpTimeProvider();
    virtual unsigned long getSecondsOfDay();
    virtual void updateTime();
};


#endif /* NtpTimeProvider_hpp */
