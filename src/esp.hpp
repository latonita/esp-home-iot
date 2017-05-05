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

#define ledOn() digitalWrite(LED_PIN,HIGH)
#define ledOff() digitalWrite(LED_PIN,LOW)
#define ledSet(x) digitalWrite(LED_PIN,x)
#define ledPulse(x) ledOn(); delayMs(2 * x / 3); ledOff(); delayMs(x / 3)
#define setupLed() pinMode(LED_PIN,OUTPUT); ledOff()

#define led2On() digitalWrite(LED_PIN2,HIGH)
#define led2Off() digitalWrite(LED_PIN2,LOW)
#define led2Set(x) digitalWrite(LED_PIN2,x)
#define led2Pulse(x) led2On(); delayMs(2 * x / 3); led2Off(); delayMs(x / 3)
#define setupLed2() pinMode(LED_PIN,OUTPUT); led2Off()

typedef void (*f_void_display_void)();
typedef void (*f_void_display_progress)(int);

typedef void (*f_mqtt_callback)(char, const char *, int);

#define MQTT_ONLINE_TOPIC MQTT_BASE_TOPIC "$online"
#define MQTT_MSG_ONLINE "online"
#define MQTT_MSG_OFFLINE "offline"
#define MQTT_ID_TOPIC MQTT_BASE_TOPIC "$id"
#define MQTT_IP_TOPIC MQTT_BASE_TOPIC "$ip"

class Esp {
public:
    typedef std::function<void (int)> THandlerFunction_Progress;

    enum WifiEvent { CONNECTING };
    typedef std::function<void (WifiEvent, int)> FWifiEventHandler;

    enum MqttEvent { CONNECT = 0, DISCONNECT, MESSAGE };
    typedef std::function<void (MqttEvent,const char*, const char*)> FMqttEventHandler;

private:
    static Esp *_me;

    char * id = NULL;
    char * hostname = NULL;

    const char * mqttBaseTopic;

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
    const char * getId();
    const char * getHostname();
    const char * getIP();
    int8_t getWifiQuality();

    void registerWifiHandler(FWifiEventHandler);
    void registerOtaHandlers(ArduinoOTAClass::THandlerFunction fStart, ArduinoOTAClass::THandlerFunction fEnd,ArduinoOTAClass::THandlerFunction_Progress fProgress, ArduinoOTAClass::THandlerFunction_Error fError);
    void startNetworkStack();

    char * mqttPublish(const char * subTopic, char * payload, bool retained);
    char * mqttPublish(const char * subTopic, byte * payload, int len, bool retained);

    void addMqttEventHandler(FMqttEventHandler handler);
    char * mqttSubscribe(const char * subTopic);

    void setup();
    void loop();

protected:
    void printVersion();
    void setupWifi();
    void setupOta();
    void setupMqtt();
    void mqttReconnect(bool firstTime);
    void mqttAnnounce();
    void mqttHeartbeat();
    void mqttSubscribe();
    void mqttCallback(char* _topic, byte* _payload, unsigned int _length);
    static void stMqttCallback(char* _topic, byte* _payload, unsigned int _length);

    Esp();
    ~Esp();
};


#endif /* esp_hpp */
