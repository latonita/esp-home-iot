//
//  esp.hpp
//  esp8266_WeatherStationDemo
//
//  Created by Anton Viktorov<author> on 04/05/2017.
//
//

#ifndef esp_hpp
#define esp_hpp

#include <stdio.h>
#include <vector>
#include <Arduino.h>


#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>  //For OTA
#include <WiFiUdp.h>      //For OTA
#include <ArduinoOTA.h>   //For OTA

#include "utils.h"
#include "config.h"

//typedef void (*f_mqtt_callback)(char, const char *, int);

#define TOPIC_SYS_ID "$id"
#define TOPIC_SYS_IP "$ip"
#define TOPIC_SYS_MAC "$mac"
#define TOPIC_SYS_ONLINE "$online"
#define TOPIC_MSG_ONLINE "online"
#define TOPIC_MSG_OFFLINE "offline"
#define TOPIC_SYS_STATS_UPTIME "$stats/uptime"
#define TOPIC_SYS_STATS_SIGNAL "$stats/signal"
#define TOPIC_SYS_STATS_FREEHEAP "$stats/freeheap"

class Esp {
public:
    typedef void (*THandlerFunction_Progress)(int);
    typedef void (*FRegularAction)(void);

    enum WifiEvent { CONNECTING };
    typedef void (*FWifiEventHandler)(WifiEvent, int);

    enum MqttEvent { CONNECT = 0, DISCONNECT, MESSAGE };
    typedef void (*FMqttEventHandler) (MqttEvent, const char*, const char*);

private:
    static Esp *_me;

    char * id = NULL;
    char * hostname = NULL;

    char * mqttBaseTopic;

    // handlers. single
    ArduinoOTAClass::THandlerFunction otafStart;
    ArduinoOTAClass::THandlerFunction otafEnd;
    ArduinoOTAClass::THandlerFunction_Progress otafProgress;
    ArduinoOTAClass::THandlerFunction_Error otafError;
    FWifiEventHandler wifiEventHandler;
    // handlers. multiple
    std::vector<FMqttEventHandler> mqttEventHandlers;

    WiFiClient wifiClient;
    WiFiServer otaServer;
    PubSubClient mqttClient;

public:
    static Esp* me();
    char * getId();
    char * getHostname();
    IPAddress getIP();
    int8_t getWifiQuality();

    void registerWifiHandler(FWifiEventHandler);
    void registerOtaHandlers(ArduinoOTAClass::THandlerFunction fStart, ArduinoOTAClass::THandlerFunction fEnd,ArduinoOTAClass::THandlerFunction_Progress fProgress, ArduinoOTAClass::THandlerFunction_Error fError);
    void startNetworkStack();

    char * mqttPublish(const char * subTopic, const char * payload, bool retained);
    char * mqttPublish(const char * subTopic, const byte * payload, int len, bool retained);

    void addMqttEventHandler(FMqttEventHandler handler);
    char * mqttSubscribe(const char * subTopic);

    void addRegularAction(unsigned int, FRegularAction);
    void setup();
    void loop();

protected:
    void printVersion();
    void setupWifi();
    void setupOta();
    void setupMqtt();
    void wifiReconnect(bool firstTime);
    void mqttReconnect(bool firstTime);
    void mqttAnnounce();
    void mqttHeartbeat();
    void mqttSubscribe();
    void mqttCallback(MqttEvent evt, const char* _topic, const char* msg);
    static void stMqttCallback(char* _topic, byte* _payload, unsigned int _len);

    Esp();
    ~Esp();
};


#endif /* esp_hpp */
