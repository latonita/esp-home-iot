#include "EspNodeBase.hpp"
#include "HttpTimeProvider.hpp"
#include <Ticker.h>

#define TRIES 5

EspNodeBase *EspNodeBase::_me = NULL;
char *EspNodeBase::_mqttBuffer = NULL;

EspNodeBase *EspNodeBase::me() {
  if (_me == NULL) {
    _me = new EspNodeBase();
  }
  return _me;
};

#define BUFFLEN 100
EspNodeBase::EspNodeBase()
    : wifiClient(), otaServer(OTA_PORT), mqttClient(wifiClient) {
  char *buff = new char[BUFFLEN];

  snprintf(buff, BUFFLEN, "%s-%06X", HOSTNAME_BASE, ESP.getChipId());
  char len = strlen(buff);

  id = new char[len + 1];
  strcpy(id, buff);

  hostname = new char[len + strlen(HOSTNAME_DOMAIN) + 1];
  strcpy(hostname, id);
  strcat(hostname, (char *)HOSTNAME_DOMAIN);

  String topic = String(MQTT_BASE_TOPIC_TEMPLATE);
  topic.replace("{id}", id);
  mqttBaseTopic = new char[topic.length() + 1];
  mqttBaseTopic[topic.length()] = 0;
  strncpy(mqttBaseTopic, topic.c_str(), topic.length());
  delete buff;
  TimeProvider *np = new HttpTimeProvider();
  TimeProvider::setProvider(np);
}

EspNodeBase::~EspNodeBase() {}

char *EspNodeBase::getId() { return id; }

char *EspNodeBase::getHostname() { return hostname; }

IPAddress EspNodeBase::getIP() { return WiFi.localIP(); }

// converts the dBm to a range between 0 and 100%
int8_t EspNodeBase::getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100) {
    return 0;
  } else if (dbm >= -50) {
    return 100;
  } else {
    return 2 * (dbm + 100);
  }
}

void EspNodeBase::setup(const char ** options = NULL, const int num = 0) { 
  printVersion(options, num); 
}

void EspNodeBase::printVersion(const char ** options = NULL, const int num = 0) {
  Serial.println();
  Serial.println(
      F("==============================================================="));
  Serial.println(
      F(": IOT Node with MQTT and OTA                                  :"));
  Serial.println(
      F(": (c) Anton Viktorov, latonita@yandex.ru, github.com/latonita :"));
  Serial.println(
      F("==============================================================="));
  Serial.printf("Compile date: %s\r\n", __DATE__ " " __TIME__);
  Serial.printf("Module id   : %s\r\n", id);
  if (options != NULL && num > 0) {
    Serial.printf("Feature set : ");
    for (int i = 1; i <= num; i++) {
      Serial.printf("%s ", options[i]);
    }
    Serial.printf("\r\n");
  }
  Serial.printf("Hostname    : %s\r\n", hostname);
  Serial.printf("Wireless    : %s\r\n", WIFI_SSID);
  Serial.printf("OTA Pass    : %s\r\n", OTA_PASSWORD);
  Serial.printf("MQTT server : %s\r\n", MQTT_SERVER);
  Serial.printf("MQTT topic  : %s\r\n", mqttBaseTopic);
  Serial.println(
      F("==============================================================="));
}

void EspNodeBase::startNetworkStack() {
  setupWifi();
  setupOta();
  setupMqtt();
}

void EspNodeBase::wifiReconnect(bool firstTime = false) {
  Serial.print(F("Connecting to WiFi.."));
  unsigned int counter = 0;
  while (WiFi.waitForConnectResult() != WL_CONNECTED && counter++ < TRIES) {
    // while (WiFi.status() != WL_CONNECTED && counter++ < RETRIES) {
    yield();
    Serial.print(".");
    if (wifiEventHandler != NULL) {
      wifiEventHandler(WifiEvent::CONNECTING,
                       counter); // small delay shallbe there
    }
    // todo: check if we cant connect for too long. options - reboot, buzz,
    // setup own wifi?
    delayMs(1000);
  }
  if (WiFi.waitForConnectResult() != WL_CONNECTED && counter == TRIES) {
    Serial.println("Connection Failed! Rebooting...");
    if (wifiEventHandler != NULL) {
      wifiEventHandler(WifiEvent::FAILURE, 0);
    }
    delayMs(1000);
    ESP.restart();
  }

  Serial.println();
  Serial.print(F("Got IP Address: "));
  Serial.println(getIP());
}

void EspNodeBase::setupWifi() {
  WiFi.persistent(false); // don't save config to flash to avoid wear of wifi
                          // config memory area
  WiFi.hostname(getHostname());
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  wifiReconnect(true);
}

void EspNodeBase::setupOta() {
  Serial.print(F("Configuring OTA device..."));
  otaServer.begin(); // Necesary to make Arduino Software autodetect OTA device
  EspNodeBase *_this = this;
  ArduinoOTA.onStart([_this]() {
    Serial.println(F("OTA starting..."));
    if (_this->otafStart != NULL)
      _this->otafStart();
  });

  ArduinoOTA.onEnd([_this]() {
    Serial.println(F("OTA update finished!"));
    Serial.println(F("Rebooting..."));
    if (_this->otafEnd != NULL)
      _this->otafEnd();
  });

  ArduinoOTA.onProgress([_this](unsigned int progress, unsigned int total) {
    int p = progress / (total / 100);
    Serial.printf("OTA in progress: %u%%\r\n", p);
    if (_this->otafProgress != NULL)
      _this->otafProgress(progress, total);
  });

  ArduinoOTA.onError([_this](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR)
      Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR)
      Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR)
      Serial.println(F("End Failed"));
    if (_this->otafError != NULL)
      _this->otafError(error);
  });

  ArduinoOTA.setPassword((const char *)OTA_PASSWORD);
  ArduinoOTA.setHostname(getHostname());
  ArduinoOTA.begin();
  Serial.println(F(" Done."));
}

void EspNodeBase::setupMqtt() {
  Serial.println(F("Configuring MQTT server..."));
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  Serial.printf("MQTT server IP: %s\r\n", MQTT_SERVER);
  Serial.println(F("MQTT configured!"));
  mqttReconnect((bool)true);
}

void EspNodeBase::mqttReconnect(bool firstTime = false) {
  int count = 0;
  while (!mqttClient.connected() && count++ < TRIES) {
    Serial.print(F("Attempting MQTT connection..."));
    if (!firstTime) {
      mqttCallback(MqttEvent::DISCONNECT, NULL, NULL, 0);
    }
    yield();

    if (mqttClient.connect(
            getHostname(),
            String(String(mqttBaseTopic) + (char *)TOPIC_SYS_ONLINE).c_str(), 0,
            1, TOPIC_MSG_OFFLINE)) {
      Serial.println(F("connected"));
      mqttCallback(MqttEvent::CONNECT, NULL, NULL, count);
      mqttAnnounce();
      mqttSubscribe();
    } else {
      Serial.printf("failed, rc=%d. Retrying in few seconds\r\n",
                    mqttClient.state());
      delayMs(1000);
    }
    if (WiFi.status() != WL_CONNECTED) { // got disconnected
      return;
    }
  }
}

void EspNodeBase::mqttAnnounce() {
  mqttPublish(TOPIC_SYS_ONLINE, TOPIC_MSG_ONLINE, true);
  mqttPublish(TOPIC_SYS_ID, getHostname(), true);
  mqttPublish(TOPIC_SYS_IP, WiFi.localIP().toString().c_str(), true);
  mqttHeartbeat();
}

void EspNodeBase::mqttHeartbeat() {
  mqttPublish(TOPIC_SYS_STATS_UPTIME, TimeProvider::me()->getUpTime(), false);
  mqttPublish(TOPIC_SYS_STATS_SIGNAL, formatInteger(getWifiQuality()), false);
  mqttPublish(TOPIC_SYS_STATS_FREEHEAP, formatInteger(ESP.getFreeHeap()),
              false);
}

void EspNodeBase::stMqttCallback(char *_topic, byte *_payload,
                                 unsigned int _len) {
  Serial.printf("Received topic: %s, payload size: %u\r\n", _topic, _len);

  char *pos = strstr((const char *)_topic, (const char *)me()->mqttBaseTopic);
  if (pos != NULL) {
    _topic += (strlen(me()->mqttBaseTopic));
    Serial.printf("Received subtopic: %s\r\n", _topic);
  }

  if (_len + 1 > ESPNODEBASE_MQTT_BUFF) {
    Serial.printf("stMqttCallback: buff %d < len %d\r\n", ESPNODEBASE_MQTT_BUFF,
                  _len + 1);
    return;
  }

  if (_mqttBuffer == NULL) {
    _mqttBuffer = new char[ESPNODEBASE_MQTT_BUFF];
  }

  if (_mqttBuffer != NULL) {
    strncpy(_mqttBuffer, (const char *)_payload, _len);
    _mqttBuffer[_len] = 0;
    Serial.printf("Received message: %s\r\n", _mqttBuffer);
    me()->mqttCallback(MqttEvent::MESSAGE, (const char *)_topic,
                       (const char *)_mqttBuffer, _len);
  } else {
    Serial.printf("stMqttCallback: Cant allocate new buffer of %u bytes\r\n",
                  _len);
  }
}

void EspNodeBase::mqttCallback(MqttEvent evt, const char *topic,
                               const char *msg, unsigned int len) {
  for (auto &fn : mqttEventHandlers) {
    fn(evt, topic, msg, len);
  }
}

void EspNodeBase::mqttSubscribe() { //?
  mqttClient.setCallback(EspNodeBase::stMqttCallback);
  //    mqttClient.subscribe("topicname");
  // resubscribe.?
}

void EspNodeBase::registerWifiHandler(FWifiEventHandler _handler) {
  wifiEventHandler = _handler;
}

void EspNodeBase::registerOtaHandlers(
    ArduinoOTAClass::THandlerFunction _fStart,
    ArduinoOTAClass::THandlerFunction _fEnd,
    ArduinoOTAClass::THandlerFunction_Progress _fProgress,
    ArduinoOTAClass::THandlerFunction_Error _fError) {
  otafStart = _fStart;
  otafEnd = _fEnd;
  otafProgress = _fProgress;
  otafError = _fError;
}

void EspNodeBase::addMqttEventHandler(FMqttEventHandler handler) {
  mqttEventHandlers.push_back(handler);
}

int EspNodeBase::mqttPublish(const char *subTopic, const char *payload,
                             bool retained = false) {
  Serial.printf("Publish to %s%s:%s\r\n", mqttBaseTopic, subTopic, payload);
  return mqttClient.publish((String(mqttBaseTopic) + subTopic).c_str(), payload,
                            retained);
}

int EspNodeBase::mqttPublish(const char *subTopic, const byte *payload, int len,
                             bool retained = false) {
  Serial.printf("Publish to %s%s:%.*s\r\n", mqttBaseTopic, subTopic, len,
                payload);
  return mqttClient.publish((String(mqttBaseTopic) + subTopic).c_str(), payload,
                            len, retained);
}

bool EspNodeBase::mqttSubscribe(const char *subTopic) {
  Serial.printf("Subscribed to %s%s\r\n", mqttBaseTopic, subTopic);
  return mqttClient.subscribe((String(mqttBaseTopic) + subTopic).c_str());
}

bool EspNodeBase::mqttUnsubscribe(const char *subTopic) {
  Serial.printf("Unsubscribed from %s%s\r\n", mqttBaseTopic, subTopic);
  return mqttClient.unsubscribe((String(mqttBaseTopic) + subTopic).c_str());
}

void EspNodeBase::addRegularAction(unsigned int seconds, FRegularAction fn) {
  if (fn != NULL) {
    Ticker *t = new Ticker();
    t->attach_ms(seconds * 1000, fn);
  }
}

TimeProvider *EspNodeBase::getTimeProvider() { return TimeProvider::me(); }

const bool EspNodeBase::isConnected() {
  return (WiFi.status() == WL_CONNECTED && mqttClient.connected());
}

void EspNodeBase::loop() {
  ArduinoOTA.handle();
  if (WiFi.status() != WL_CONNECTED) {
    wifiReconnect();
  }
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  static ElapsedMillis timeElapsed;
  if (timeElapsed > (unsigned long)(BASE_HEARTBEAT_SECS * 1000)) {
    mqttHeartbeat();
    timeElapsed.rearm();
  }
  mqttClient.loop();
}
