//
//  esp.cpp
//  esp8266_WeatherStationDemo
//
//  Created by <author> on 04/05/2017.
//
//

#include "esp.hpp"
#include <Ticker.h>

Esp* Esp::_me = NULL;

Esp* Esp::me() {
    if (_me == NULL) {
        _me = new Esp();
    }
    return _me;
};

Esp::Esp() : wifiClient(), otaServer(8266), mqttClient(wifiClient) {
    char buff[100];
    snprintf(buff, 100, "%s_%06X", (char*)HOSTNAME_BASE, ESP.getChipId());
    char len = strlen(buff);

    id = new char[len + 1];
    strcpy(id, buff);

    hostname = new char[len + strlen((char*)HOSTNAME_DOMAIN) + 1];
    strcpy(hostname, id);
    strcat(hostname, (char*)HOSTNAME_DOMAIN);

    snprintf(buff, 100, MQTT_BASE_TOPIC_TEMPLATE, id);
    mqttBaseTopic = new char(strlen(buff)+1);
    strcpy(mqttBaseTopic, buff);
}

Esp::~Esp() {}

char * Esp::getId() {
    return id;
}

char * Esp::getHostname() {
    return hostname;
}

IPAddress Esp::getIP() {
    return WiFi.localIP();
}

// converts the dBm to a range between 0 and 100%
int8_t Esp::getWifiQuality() {
    int32_t dbm = WiFi.RSSI();
    if(dbm <= -100) {
        return 0;
    } else if(dbm >= -50) {
        return 100;
    } else {
        return 2 * (dbm + 100);
    }
}

void Esp::setup() {
    printVersion();
}

void Esp::printVersion() {
    Serial.println();
    Serial.println("===============================================================");
    Serial.println(": MQTT Pulse counter and MQTT display with OTA                :");
    Serial.println(": (c) Anton Viktorov, latonita@yandex.ru, github.com/latonita :");
    Serial.println("===============================================================");
    Serial.printf("Compile date: %s\r\n", __DATE__ " " __TIME__);
    Serial.printf("Module id   : %s\r\n", id);
    Serial.printf("Hostname    : %s\r\n", hostname);
    Serial.printf("Wireless    : %s\r\n", WIFI_SSID);
    Serial.printf("MQTT server : %s\r\n", MQTT_SERVER);
    Serial.printf("MQTT topic  : %s\r\n", mqttBaseTopic);
    Serial.println("===============================================================");
}

void Esp::startNetworkStack() {
    setupWifi();
    setupOta();
    setupMqtt();
}

void Esp::wifiReconnect(bool firstTime = false) {
    Serial.print("Connecting to WiFi..");
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED) {
        yield();
        Serial.print(".");
        if (wifiEventHandler != NULL) {
            wifiEventHandler(WifiEvent::CONNECTING, counter); // small delay shallbe there
        }
        //todo: check if we cant connect for too long. options - reboot, buzz, setup own wifi?
        delayMs(1000);
    }
    Serial.println();
    Serial.print("Got IP Address: ");
    Serial.println(getIP());
}

void Esp::setupWifi() {
    WiFi.hostname(getHostname());
    WiFi.begin(WIFI_SSID, WIFI_PWD);
    wifiReconnect(true);
}

void Esp::setupOta(){
    Serial.print("Configuring OTA device...");
    otaServer.begin(); //Necesary to make Arduino Software autodetect OTA device
    Esp* _this = this;
    ArduinoOTA.onStart([_this]() {
        Serial.println("OTA starting...");
        if (_this->otafStart != NULL) _this->otafStart();
    });

    ArduinoOTA.onEnd([_this]() {
        Serial.println("OTA update finished!");
        Serial.println("Rebooting...");
        if (_this->otafEnd != NULL) _this->otafEnd();
    });

    ArduinoOTA.onProgress([_this](unsigned int progress, unsigned int total) {
        int p = progress / (total / 100);
        Serial.printf("OTA in progress: %u%%\r\n", p );
        if (_this->otafProgress != NULL) _this->otafProgress(progress, total);
    });

    ArduinoOTA.onError([_this](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
        if (_this->otafError != NULL) _this->otafError(error);
    });

    ArduinoOTA.setPassword((const char *)OTA_PASSWORD);
    ArduinoOTA.setHostname(getHostname());
    ArduinoOTA.begin();
    Serial.println(" Done.");
}

void Esp::setupMqtt() {
    Serial.println("Configuring MQTT server...");
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    Serial.printf("MQTT server IP: %s\r\n", MQTT_SERVER);
    Serial.println("MQTT configured!");
    mqttReconnect((bool)true);
}

void Esp::mqttReconnect(bool firstTime = false) {
//  backToSystemPage();
// Loop until we're reconnected
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (!firstTime) {
            mqttCallback(MqttEvent::DISCONNECT, NULL, NULL);
        }
//    updateDisplay();
        yield();

        // boolean connect (clientID, willTopic, willQoS, willRetain, willMessage)
        if (mqttClient.connect(getHostname(), String(String(mqttBaseTopic) + (char*)TOPIC_SYS_ONLINE).c_str(), 0, 1, TOPIC_MSG_OFFLINE)) {
            Serial.println("connected");
            mqttCallback(MqttEvent::CONNECT, NULL, NULL);
            mqttAnnounce();
            //mqttSubscribe();
        } else {
            Serial.printf("failed, rc=%d. Retrying in few seconds\r\n",mqttClient.state());
//      updateDisplay();
            delayMs(1000);
        }
    }
}

void Esp::mqttAnnounce() {
    mqttPublish(TOPIC_SYS_ONLINE, TOPIC_MSG_ONLINE, true);
    mqttPublish(TOPIC_SYS_ID, getHostname(), true);
    mqttPublish(TOPIC_SYS_IP, WiFi.localIP().toString().c_str(), true);
}

void Esp::mqttHeartbeat() {
    mqttPublish(TOPIC_SYS_STATS_UPTIME, getUpTime(), false);
    mqttPublish(TOPIC_SYS_STATS_SIGNAL, formatInteger(getWifiQuality()), false);
    mqttPublish(TOPIC_SYS_STATS_FREEHEAP, formatInteger(ESP.getFreeHeap()), false);
}

void Esp::stMqttCallback(char* _topic, byte* _payload, unsigned int _len) {
    char msg[_len + 1];
    strlcpy(msg, (char*)_payload, _len + 1);
    me()->mqttCallback(MqttEvent::MESSAGE, (const char*)_topic,(const char*) msg);
}

void Esp::mqttCallback(MqttEvent evt, const char* topic, const char* msg) {
    for (auto &fn : mqttEventHandlers) {
        fn(evt,topic,msg);
    }
}

void Esp::mqttSubscribe() {//?
    mqttClient.setCallback(Esp::stMqttCallback);
    mqttClient.subscribe("topicname");
}


void Esp::registerWifiHandler(FWifiEventHandler _handler) {
    wifiEventHandler = _handler;
}

void Esp::registerOtaHandlers(ArduinoOTAClass::THandlerFunction _fStart, ArduinoOTAClass::THandlerFunction _fEnd, ArduinoOTAClass::THandlerFunction_Progress _fProgress, ArduinoOTAClass::THandlerFunction_Error _fError){
    otafStart = _fStart;
    otafEnd = _fEnd;
    otafProgress = _fProgress;
    otafError = _fError;
}

void Esp::addMqttEventHandler(FMqttEventHandler handler) {
    mqttEventHandlers.push_back(handler);
}

char * Esp::mqttPublish(const char * subTopic, const char * payload, bool retained = false) {
    mqttClient.publish((String(mqttBaseTopic) + subTopic).c_str(), payload, retained);
}

char * Esp::mqttPublish(const char * subTopic, const byte * payload, int len, bool retained = false) {
    mqttClient.publish((String(mqttBaseTopic) + subTopic).c_str(), payload, len, retained);
}

char * Esp::mqttSubscribe(const char * subTopic) {
    mqttClient.subscribe((String(mqttBaseTopic) + subTopic).c_str());
}

void Esp::addRegularAction(unsigned int seconds, FRegularAction fn) {
  if (fn != NULL){
    Ticker *t = new Ticker();
    t->attach_ms(seconds*1000, fn);
  }
}

void Esp::loop() {
    ArduinoOTA.handle();
    if (WiFi.status() != WL_CONNECTED) {
      wifiReconnect();
    }
    if (!mqttClient.connected()) {
        mqttReconnect();
    }
    mqttClient.loop();
}
