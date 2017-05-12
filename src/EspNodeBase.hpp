//
//  esp.hpp
//  esp8266_WeatherStationDemo
//
//  Created by Anton Viktorov<author> on 04/05/2017.
//
//

#ifndef EspNodeBase_hpp
#define EspNodeBase_hpp

#include <stdio.h>
#include <vector>
#include <Arduino.h>


#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>  //For OTA
#include <WiFiUdp.h>      //For OTA
#include <ArduinoOTA.h>   //For OTA
#include <TimeClient.h>

#include "utils.h"
#include "config.h"

#define TOPIC_SYS_ID "$id"
#define TOPIC_SYS_IP "$ip"
#define TOPIC_SYS_MAC "$mac"
#define TOPIC_SYS_ONLINE "$online"
#define TOPIC_MSG_ONLINE "online"
#define TOPIC_MSG_OFFLINE "offline"
#define TOPIC_SYS_STATS_UPTIME "$stats/uptime"
#define TOPIC_SYS_STATS_SIGNAL "$stats/signal"
#define TOPIC_SYS_STATS_FREEHEAP "$stats/freeheap"



//#define MQTT_BASE_TOPIC "world/p14/sensors/entrance/"





//
// about mqtt naming
//
// location/myhome/entrance/sensor/entrance/$online = online/offline
// location/myhome/entrance/sensor/entrance/$id
// location/myhome/entrance/sensor/entrance/$ip
// location/myhome/entrance/sensor/entrance/$stats/uptime = seconds
// location/myhome/entrance/sensor/entrance/$stats/signal = signal strength
// location/myhome/entrance/sensor/entrance/$fw = firmware string

// location/myhome/entrance/sensor/entrance/power/pulses_raw  = pulses;period
// location/myhome/entrance/sensor/entrance/power/instant  = instant watts
// location/myhome/entrance/sensor/entrance/power/usage = wh since last time
// location/myhome/entrance/sensor/entrance/temperature  = temp
// location/myhome/entrance/sensor/entrance/humidity  = temp
// location/myhome/entrance/sensor/entrance/doorbell = dtm;photo

// myhome/sensors/bathroom/water/pulses_raw = pulses_cold;pulses_hot;period

// data to show:
// myhome/presentation/power ///// ????
// myhome/presentation/alert/

//to read: http://tinkerman.cat/mqtt-topic-naming-convention/
//https://harizanov.com/2014/09/mqtt-topic-tree-structure-improvements/
//https://forum.mysensors.org/topic/4341/mqtt-messages-and-sensor-tree/4
//https://hackaday.io/project/7342-mqopen/log/33302-mqtt-topic-structure-redesign

//#define MQTT_BASE_TOPIC "world/p14/sensors/entrance/"
class EspNodeBase {
public:
    typedef void (* THandlerFunction_Progress)(int);
    typedef void (* FRegularAction)(void);

    enum WifiEvent { CONNECTING };
    typedef void (* FWifiEventHandler)(WifiEvent, int);

    enum MqttEvent { CONNECT = 0, DISCONNECT, MESSAGE };
    typedef void (* FMqttEventHandler)(MqttEvent, const char *, const char *);

private:
    static EspNodeBase * _me;

    char * id = NULL;
    char * hostname = NULL;

    char * mqttBaseTopic = NULL;

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
    static EspNodeBase * me();
    char * getId();
    char * getHostname();
    IPAddress getIP();
    int8_t getWifiQuality();

    void registerWifiHandler(FWifiEventHandler);
    void registerOtaHandlers(ArduinoOTAClass::THandlerFunction fStart, ArduinoOTAClass::THandlerFunction fEnd,ArduinoOTAClass::THandlerFunction_Progress fProgress, ArduinoOTAClass::THandlerFunction_Error fError);
    void startNetworkStack();

    int mqttPublish(const char * subTopic, const char * payload, bool retained);
    int mqttPublish(const char * subTopic, const byte * payload, int len, bool retained);

    void addMqttEventHandler(FMqttEventHandler handler);
    bool mqttSubscribe(const char * subTopic);

    void addRegularAction(unsigned int, FRegularAction);

    TimeProvider * getTimeProvider();

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
    void mqttCallback(MqttEvent evt, const char * _topic, const char * msg);
    static void stMqttCallback(char * _topic, byte * _payload, unsigned int _len);

    EspNodeBase();
    ~EspNodeBase();
};


#endif /* esp_hpp */
