#ifndef __MYEVA_H
#define __MYEVA_H

#include <stdint.h>
#include "Arduino.h"
#include "Ethercat.h"
#include "VirtualSerial.h"
#include "VirtualLcm.h"
#include "VirtualMpg.h"
#include "VirtualKeypad.h"
#include "VirtualLcd.h"
#include "VirtualMic.h"
#include "VirtualSpeaker.h"
#include "VirtualServo.h"
#include "VirtualCNC.h"
#include "VirtualEncoder.h"
//If ChatGPT use the C++11 feature on for loops, will need to include initializer_list header
#include <initializer_list>

enum MyVirtualError {
    EVAERR_NO_CONFIG_FILE = -1000,
    EVAERR_NO_ENOUGH_MEMORY = -1001
};

extern EthercatDevice_QECR11MP3S Slave0;

class MyEVA {

    public:
        MyEVA();
        int begin();
        void end();

        int digitalWrite(uint32_t pin, uint8_t val);
        int digitalRead(uint32_t pin);
        int tone(uint8_t pin, uint32_t freq, uint32_t duration);
        int counterRead(uint32_t pin);
        int setCounterMode(int alias, uint8_t mode, uint16_t period_ms=1);
        double getUsVoltage(int alias);
        double getUpVoltage(int alias);
        double getIsCurrent(int alias);
        double getIpCurrent(int alias);
        double getTemperature(int alias);
};

extern MyEVA EVA;
extern EthercatMaster EcatMaster;

extern MyVirtualCNC VirtualCNC1;
extern MyVirtualCNC VirtualCNC2;
extern MyVirtualCNC VirtualCNC3;
extern MyVirtualCNC VirtualCNC4;
extern MyVirtualServo VirtualServo1;
extern MyVirtualServo VirtualServo2;
extern MyVirtualServo VirtualServo3;
extern MyVirtualServo VirtualServo4;
extern MyVirtualServo VirtualServo5;
extern MyVirtualServo VirtualServo6;
extern MyVirtualServo VirtualServo7;
extern MyVirtualServo VirtualServo8;
extern MyVirtualEncoder VirtualEncoder1;
extern MyVirtualEncoder VirtualEncoder2;
extern MyVirtualEncoder VirtualEncoder3;
#endif