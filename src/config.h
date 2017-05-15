/***************************
 * Connection params
 **************************/
#define HOSTNAME_BASE "espnode"   // used to produce id and hostname
#define HOSTNAME_DOMAIN ".lan"    // "espnode-xxxx.lan"

#define WIFI_SSID "LAWIRELESS"    // my wireless network. use yours..
#define WIFI_PWD "MegaPass!"      // your super mega secret password for wifi. try this one. but not sure it will work ;)))
#define OTA_PASSWORD "Secret!"    // your super mega secret password for OTA update

#define MQTT_SERVER "192.168.3.3" // MQTT server
#define MQTT_PORT 1883

//#define MQTT_BASE_TOPIC_TEMPLATE "location/myhome/entrance/sensor/{id}/"
#define MQTT_BASE_TOPIC_TEMPLATE "home-iot-node/{id}/"
#define MQTT_DEVICE_CLASS_TEMPLATE "{class}/{type}"

// class := {sensor, display}
// sensor type := {temperature, humidity, power, water, binary}
// sensor/temperature
// display type:= {..}


#define SERIAL_BAUD_RATE 74880

/***************************
 * ALL PINS HERE
 **************************/
#define POWER_PULSE_PIN 14    // kwh pulses
#define WATER_HOT_PULSE_PIN 13    // kwh pulses
#define WATER_COLD_PULSE_PIN 16    // kwh pulses

#define ANOTHER_PIN 13  // one more signal pin available, will be shorted to ground by external device
#define LED_PIN 15
#define LED_PIN2 2
#define DHT_PIN 12

#define SDA_PIN 5
#define SDC_PIN 4

#define I2C_DISPLAY_ADDRESS 0x3c

#define LED1 LED_PIN
#define LED2 LED_PIN2


/***************************
 * Data collection periods
 **************************/
#define MQTT_DATA_COLLECTION_PERIOD_SECS 5 * 60 // time to collect data before posting to mqtt
#define FORECAST_UPDATE_INTERVAL_SECS 30 * 60 // Update weather forecast every 30 minutes
#define DHT_UPDATE_INTERVAL_SECS 1 * 60 // read from DHT sensor

#define BASE_HEARTBEAT MQTT_DATA_COLLECTION_PERIOD_SECS

/***************************
 * Features on/off
 **************************/
#define DHT_ON
//#define THINGSPEAK_ON
#define WEATHER_ON
#define POWER_ON

/***************************
 * Other
 **************************/
#define UTC_OFFSET 3

#define POWER_PULSES_PER_WATT_HOUR 0.64 //640 pulses = 1000 W*h

#define WATER_COLD_PULSES_PER_LITERS 1
#define WATER_HOT_PULSES_PER_LITERS 1
