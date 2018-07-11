#include "HttpTimeProvider.hpp"
#include <ESP8266WiFi.h>
#include <time.h>

HttpTimeProvider::HttpTimeProvider() : TimeProvider() {
}

unsigned long HttpTimeProvider::getSecondsOfDay(){
    return getCurrentEpochWithUtcOffset() % SECONDS_IN_A_DAY;
}

const char * HttpTimeProvider::getDateString(){
    static char buff[32];
    long e = getCurrentEpochWithUtcOffset();
    struct tm * t = localtime(&e);
    snprintf(buff, 31, "%04d-%02d-%02d",t->tm_year+1900, t->tm_mon+1, t->tm_mday);
    return buff;
}

long HttpTimeProvider::getCurrentEpoch() {
  return localEpoc + ((millis() - localMillisAtUpdate) / 1000);
}

long HttpTimeProvider::getCurrentEpochWithUtcOffset() {
  return getCurrentEpoch() + 3600 * myUtcOffset;
}

void HttpTimeProvider::updateTime() {
  Serial.println("HttpTimeProvider::updateTime()");
  WiFiClient client;
  if (!client.connect(HTTP_TIME_URL, HTTP_TIME_PORT)) {
    Serial.println("updateTime() connection failed");
    return;
  }

  // This will send the request to the server
  client.print(HTTP_TIME_REQUEST);
  int repeatCounter = 0;
  while(!client.available() && repeatCounter < 10) {
    delay(1000);
    Serial.print(".");
    repeatCounter++;
  }

  String line;

  int size = 0;
  client.setNoDelay(false);
  while(client.connected()) {
    while((size = client.available()) > 0) {
      line = client.readStringUntil('\n');
      line.toUpperCase();
      // date: Thu, 19 Nov 2015 20:25:40 GMT
      if (line.startsWith("DATE: ")) {
        struct tm tm = {0};
        char *s = strptime(line.substring(11, 34).c_str(), "%d %b %Y %H:%M:%S", &tm);
        if (s == NULL) {
            Serial.printf("Cannot parse date\r\n");
            return;
        } else {
            Serial.printf("HttpTimeProvider() got datetime: ");
            Serial.printf("year: %d; month: %d; day: %d; ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            Serial.printf("hour: %d; minute: %d; second: %d\r\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
        localEpoc = mktime(&tm);
        Serial.println(localEpoc);
        localMillisAtUpdate = millis();
      }
    }
  }
}
