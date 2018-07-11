#include "EspNodeService.hpp"
#include "../Config.h"
#include <WString.h>


const char * EspNodeServiceBase::ClassNames[] = {"sensor", "display"};
const char * EspNodeServiceBase::TypeNames[] = {"dht", "power", "water", "binary", "text"};

EspNodeServiceBase::EspNodeServiceBase(EspNodeServiceBase::Class _class, EspNodeServiceBase::Type _type) : serviceClass(_class), serviceType(_type) {
    String temp = String((char*)MQTT_DEVICE_CLASS_TEMPLATE);
    temp.replace("{class}", EspNodeServiceBase::ClassNames[serviceClass]);
    temp.replace("{type}", EspNodeServiceBase::TypeNames[serviceType]);
    prefix = new char[temp.length() + 1];
    strcpy(prefix,temp.c_str());
}
EspNodeServiceBase::~EspNodeServiceBase() {}

const char* EspNodeServiceBase::getPrefix() {
    return prefix;
};

DHTService::DHTService() : EspNodeServiceBase(EspNodeServiceBase::Class::SENSOR, EspNodeServiceBase::Type::DHT) {}
