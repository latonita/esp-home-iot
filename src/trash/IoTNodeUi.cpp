// //
// //  OledUi.cpp
// //  esp-home-iot
// //
// //  Created by <author> on 08/05/2017.
// //
// //
//
// #include "IoTNodeUi.hpp"
// #include "config.h"
// #include "utils.h"
// #include "res/WeatherStationImages.h"
//
// IoTNodeUi::IoTNodeUi() {}
// IoTNodeUi::~IoTNodeUi() {}
//
//
// void IoTNodeUi::initHardware(){
//     Serial.println(F("OLED hardware init..."));
//     display = new SSD1306Wire(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
//     display->init();
//     display->clear();
//     display->display();
//     //display->flipScreenVertically();
//     display->setFont(ArialMT_Plain_10);
//     display->setTextAlignment(TEXT_ALIGN_CENTER);
//     display->setContrast(180);
// }
//
// void IoTNodeUi::initUi() {
//     Serial.println(F("UI Init..."));
//     ui = new OLEDDisplayUi(display);
//     ui->setTargetFPS(30);
//     //ui->setActiveSymbol(activeSymbole);
//     //ui->setInactiveSymbol(inactiveSymbole);
//     ui->setActiveSymbol(emptySymbol);    // Hack until disableIndicator works
//     ui->setInactiveSymbol(emptySymbol);   // - Set an empty symbol
//     ui->disableIndicator();
//
//     // You can change this to TOP, LEFT, BOTTOM, RIGHT
//     ui->setIndicatorPosition(BOTTOM);
//     // Defines where the first frame is located in the bar.
//     ui->setIndicatorDirection(LEFT_RIGHT);
//     // You can change the transition that is used
//     // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
//     ui->setFrameAnimation(SLIDE_LEFT);
//
//
//     ui->setFrames(&frames[0], frames.size());
// //      ui->setOverlays(overlays, numberOfOverlays);
//     // Inital UI takes care of initalising the display too.
//     ui->init();
//     updateData();
// }
//
// void IoTNodeUi::displayProgress(int percentage, const char * label) {
//     display->clear();
//     display->setTextAlignment(TEXT_ALIGN_CENTER);
//     display->setFont(ArialMT_Plain_10);
//     display->drawString(64, 10, label);
//     display->drawProgressBar(2, 28, 124, 10, percentage);
//     display->display();
// }
//
// void IoTNodeUi::displayEndlessProgress(const char * msg, bool finished = false) {
//     static int counter = 0;
//     display->clear();
//     display->setFont(ArialMT_Plain_10);
//     display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
//     display->drawString(DISPLAY_WIDTH / 2, 10, msg);
//     display->drawXbm(46, 30, 8, 8, finished || (counter % 3 == 0) ? activeSymbole : inactiveSymbole);
//     display->drawXbm(60, 30, 8, 8, finished || (counter % 3 == 1) ? activeSymbole : inactiveSymbole);
//     display->drawXbm(74, 30, 8, 8, finished || (counter % 3 == 2) ? activeSymbole : inactiveSymbole);
//     if (finished) display->drawString(DISPLAY_WIDTH / 2, 50, "Done");
//     display->display();
//     delayMs(finished ? 500 : 100);
//     counter++;
// }
//
// void IoTNodeUi::updateData() {
//     Serial.println("Data update...");
//     int num = parts.size();
//     for (int i = 0; i < num; ++i) {
//         char buff[32];
//         snprintf_P(buff, 32, "Updating %s...\r\n",parts[i]->getName());
//         Serial.println(buff);
//         displayProgress((i + 1) * 100 / i,buff);
//         parts[i]->updateData();
//     }
// }
//
// void IoTNodeUi::addUiPart(IoTNodeUi::Part * p){
//     parts.push_back(p);
//     p->registerUi(this);
// }
//
// void IoTNodeUi::addFrame(FrameCallback frame){
//     frames.push_back(frame);
// }
