// //
// //  OledUi.hpp
// //  esp-home-iot
// //
// //  Created by <author> on 08/05/2017.
// //
// //
//
// #ifndef IoTNodeUi_hpp
// #define IoTNodeUi_hpp
//
// #include <stdio.h>
// #include <SSD1306Wire.h>
// #include <OLEDDisplayUi.h>
// #include <vector>
// #include "config.h"
//
// class IoTNodeUi {
// public:
//     class Part {
// public:
//         virtual char * getName() = 0;
//         virtual void updateData() = 0;
//         virtual void draw() = 0;
//         virtual void registerUi(IoTNodeUi *) = 0;
//     };
//
// protected:
//     SSD1306Wire * display;
//     OLEDDisplayUi * ui;
//     std::vector<FrameCallback> frames;
//     std::vector<Part *> parts;
//
// public:
//     IoTNodeUi();
//     virtual ~IoTNodeUi();
//
//     virtual void initHardware();
//     virtual void initUi();
//
//     void addUiPart(Part *);
//
//     void updateData();
//
//     void addFrame(FrameCallback);
//
//     void displayProgress(int percentage, const char * label);
//     void displayEndlessProgress(const char * msg, bool finished);
//
// };
//
//
// #endif /* IoTNodeUi_hpp */
