#ifndef HttpTimeProvider_hpp
#define HttpTimeProvider_hpp

#include <stdio.h>
#include "../Sys/Utils.h"
#include "../Config.h"

#define HTTP_TIME_PORT 80
#define HTTP_TIME_URL "tank.local"
#define HTTP_TIME_REQUEST "GET / HTTP/1.1\r\nHost: " HTTP_TIME_URL "\r\nConnection: close\r\n\r\n"


class HttpTimeProvider : public TimeProvider {
private:
  float myUtcOffset = UTC_OFFSET;
  long localEpoc = 0;
  long localMillisAtUpdate;
public:
    HttpTimeProvider();
    virtual unsigned long getSecondsOfDay();
    virtual const char* getDateString();
    virtual void updateTime();

    long getCurrentEpoch();
    long getCurrentEpochWithUtcOffset();
};


#endif /* HttpTimeProvider_hpp */
