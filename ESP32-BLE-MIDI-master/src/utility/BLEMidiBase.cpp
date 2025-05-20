#include <Arduino.h>
#include "BLEMidiBase.h"

void BLEMidi::begin(const std::string deviceName)
{
    this->deviceName = deviceName;
    NimBLEDevice::init(deviceName);
}

void BLEMidi::end() {
    NimBLEDevice::deinit();
}

bool BLEMidi::isConnected()
{
    return connected;
}
