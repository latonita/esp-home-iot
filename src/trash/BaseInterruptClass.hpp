// //
// //  BaseInterruptClass.hpp
// //  esp-home-iot
// //
// //  Created by Anton Viktorov on 2017/05/10.
// //
// //
//
// #ifndef BaseInterruptClass_hpp
// #define BaseInterruptClass_hpp
//
// #include <stdio.h>
// #include <utility>
//
// #include <Arduino.h>
//
// #define CALL_MEMBER_FN(object,ptrToMember)  ((object)->*(ptrToMember))
// //#define BASE_ISR(x) baseIsr ## x() {(*isrPairs[x].first)(isrPairs[x].second);}
// //#define MAX_ISR 17
// class BaseInterruptClass {
// public:
//     typedef void (BaseInterruptClass::*IsrFunc)(void);
//
//
// private:
//     static std::pair<BaseInterruptClass*, IsrFunc> isrPairs[2];
//
//     static void isr13(void) {
//         BaseInterruptClass * p = isrPairs[0].first;
//         IsrFunc fn = isrPairs[0].second;
//         CALL_MEMBER_FN(p,fn)();
//     };
//
//     static void isr14(void) {
//         BaseInterruptClass * p = isrPairs[1].first;
//         IsrFunc fn = isrPairs[1].second;
//         CALL_MEMBER_FN(p,fn)();
//     };
//
// public:
//     static void attachInterrupt(uint8_t pin, BaseInterruptClass* p, IsrFunc fn, int mode) {
//         isrPairs[pin].first = p;
//         isrPairs[pin].second = fn;
//         pinMode(pin, INPUT);
//         switch (pin) {
//             case 13: ::attachInterrupt(13, &BaseInterruptClass::isr13, mode); break;
//             case 14: ::attachInterrupt(14, &BaseInterruptClass::isr14, mode); break;
//             default:
//                 Serial.printf("ERROR: ISR on pin %d is not supported!!\r\n", pin);
//         }
//     };
//
// };
//
//
// #endif /* BaseInterruptClass_hpp */
