//
//  esp.cpp
//  esp8266_WeatherStationDemo
//
//  Created by <author> on 04/05/2017.
//
//

#include "esp.hpp"

Esp* Esp::_me = NULL;

Esp* Esp::me() {
    if (_me == NULL) {
        _me = new Esp();
    }
    return _me;
};

Esp::Esp() : mqttBaseTopic(MQTT_BASE_TOPIC), wifiClient(), otaServer(8266), mqttClient(wifiClient) {}

Esp::~Esp() {}

const char * Esp::getId() {
    if (id == NULL) {
        char buff[20];
        snprintf(buff, 20, "%s_%06X", HOSTNAME_BASE, ESP.getChipId());
        id = new char[strlen(buff) + 1];
        strcpy(buff, id);
    }
    return id;
}
const char * Esp::getHostname() {
    if (hostname == NULL) {
        int len = strlen(getId()) + strlen((char*)HOSTNAME_DOMAIN) + 1;
        hostname = new char[len];
        strcpy(hostname, getId());
        strcat(hostname, (char*)HOSTNAME_DOMAIN);
    }
    return hostname;

}

const char * Esp::getIP() {
    return WiFi.localIP().toString().c_str();
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
    Serial.printf("Module id: %s\r\n", getHostname());
    Serial.printf("Wireless %s\r\n", WIFI_SSID);
    Serial.printf("MQTT server %s\r\n", MQTT_SERVER);
    Serial.printf("MQTT topic %s\r\n", MQTT_BASE_TOPIC);
    Serial.println("===============================================================");
}

void Esp::startNetworkStack() {
    setupWifi();
    setupOta();
    setupMqtt();
}

void Esp::setupWifi() {
    WiFi.hostname(getHostname());
    WiFi.begin(WIFI_SSID, WIFI_PWD);
    Serial.print("Connecting to WiFi..");
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED) {
//        led2Pulse(250);
        yield();
        Serial.print(".");
        if (wifiEventHandler != NULL) {
            wifiEventHandler(WifiEvent::CONNECTING, counter); // small delay shallbe there
        }
        counter++;
        //todo: check if we cant connect for too long. options - reboot, buzz, setup own wifi?
    }
    Serial.println();
    Serial.print("Got IP Address: ");
    Serial.println(getIP());
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
        led2Pulse(250);
//    updateDisplay();
        yield();

        // boolean connect (clientID, willTopic, willQoS, willRetain, willMessage)
        if (mqttClient.connect(getHostname(), MQTT_ONLINE_TOPIC, 0, 1, MQTT_MSG_OFFLINE)) {
            Serial.println("connected");
            mqttAnnounce();
            mqttSubscribe();
        } else {
            Serial.printf("failed, rc=%d. Retrying in few seconds\r\n",mqttClient.state());
//      updateDisplay();
            for (int i = 0; i < 5; i++) {
                led2Pulse(250);
            }
        }
    }
}

void Esp::mqttAnnounce() {
    mqttClient.publish(MQTT_ONLINE_TOPIC, MQTT_MSG_ONLINE, true);
    mqttClient.publish(MQTT_ID_TOPIC, getHostname(), true);
    mqttClient.publish(MQTT_IP_TOPIC, getIP(), true);
}

void Esp::mqttHeartbeat() {
    mqttPublish("$stats/uptime", formatInteger(millis() / 1000), false);
    mqttPublish("$stats/signal", formatInteger(getWifiQuality()), false);
}

void Esp::stMqttCallback(char* _topic, byte* _payload, unsigned int _length) {
    me()->mqttCallback(_topic, _payload, _length);
}

void Esp::mqttSubscribe() {
    mqttClient.setCallback(Esp::stMqttCallback);
    mqttClient.subscribe("topicname");
}

void Esp::mqttCallback(char* _topic, byte* _payload, unsigned int _length) {
    //todo

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

char * Esp::mqttPublish(const char * subTopic, char * payload, bool retained = false) {
    mqttClient.publish((String(mqttBaseTopic) + subTopic).c_str(), payload, retained);
}

char * Esp::mqttPublish(const char * subTopic, byte * payload, int len, bool retained = false) {
    mqttClient.publish((String(mqttBaseTopic) + subTopic).c_str(), payload, len, retained);
}

char * Esp::mqttSubscribe(const char * subTopic) {
    mqttClient.subscribe((String(mqttBaseTopic) + subTopic).c_str());
}

void Esp::loop() {
    ArduinoOTA.handle();
    if (!mqttClient.connected()) {
        mqttReconnect();
    }
    mqttClient.loop();
}
