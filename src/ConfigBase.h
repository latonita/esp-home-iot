/***************************
 * Connection params
 **************************/
#define HOSTNAME_BASE "espnode"   // used to produce id and hostname
#define HOSTNAME_DOMAIN ".local"    // "espnode-xxxx.local"

#define WIFI_SSID "LAWIRELESS"    // my wireless network. use yours.. i'm using env variables to set real ssid and password
#define WIFI_PWD "MegaPass!"      // your super mega secret password for wifi. i'm using env variables to set real ssid and password
#define OTA_PASSWORD "Secret!"    // your super mega secret password for OTA update. i'm using env variables to set real ssid and password

#define MQTT_SERVER "192.168.3.3" // MQTT server
#define MQTT_PORT 1883

/***************************
 * Data collection periods
 **************************/
#define MQTT_DATA_COLLECTION_PERIOD_SECS 1 * 60 // time to collect data before posting to mqtt
#define BASE_HEARTBEAT_SECS MQTT_DATA_COLLECTION_PERIOD_SECS
#define MQTT_RESUBSCRIBE_SECS 60 * 60 * 24 // resubscribe every 24 hrs, pubsubclient stops receiving subscription for some reason

//#define MQTT_BASE_TOPIC_TEMPLATE "location/myhome/entrance/sensor/{id}/"
#define MQTT_BASE_TOPIC_TEMPLATE "home-iot-node/{id}/"
#define MQTT_DEVICE_CLASS_TEMPLATE "{class}/{type}"

// class := {sensor, display}
// sensor type := {temperature, humidity, power, water, binary}
// sensor/temperature
// display type:= {..}


