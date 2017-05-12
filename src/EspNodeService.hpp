//
//  EspNodeService.hpp
//  esp-home-iot
//
//  Created by Anton Viktorov on 2017/05/11.
//
//

#ifndef EspNodeService_hpp
#define EspNodeService_hpp

#include <stdio.h>


class EspNodeServiceBase {
private:
    char * prefix;

public:
    enum Class { SENSOR = 0, DISPLAY = 1 };
    static const char * ClassNames[];
    enum Type { DHT = 0, POWER, WATER, BINARY, TEXT };
    static const char * TypeNames[];

    Class serviceClass;
    Type serviceType;
    const char * getPrefix();

    EspNodeServiceBase(Class,Type);
    ~EspNodeServiceBase();

    virtual char * getData() = 0;
    virtual void subscribe() = 0;

protected:

};

class DHTService : public EspNodeServiceBase {
public:
    DHTService();
    ~DHTService();
};


#endif /* EspNodeService_hpp */
