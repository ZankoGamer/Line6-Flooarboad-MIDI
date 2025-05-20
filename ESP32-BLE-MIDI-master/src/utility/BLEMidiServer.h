#ifndef BLE_MIDI_SERVER_H
#define BLE_MIDI_SERVER_H
#include "BLEMidiBase.h"
// Server class now uses NimBLEServerCallbacks and NimBLECharacteristicCallbacks.
class BLEMidiServerClass : public BLEMidi, public NimBLEServerCallbacks {
public:
    void begin(const std::string deviceName);
    void setOnConnectCallback(void (*const onConnectCallback)());
    void setOnDisconnectCallback(void (*const onDisconnectCallback)());
    
private:
    virtual void sendPacket(uint8_t *packet, uint8_t packetSize) override;
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
    
    void (*onConnectCallback)() = nullptr;
    void (*onDisconnectCallback)() = nullptr;
    NimBLECharacteristic* pCharacteristic = nullptr;
};

class CharacteristicCallback: public NimBLECharacteristicCallbacks {
public:
    CharacteristicCallback(std::function<void(uint8_t*, uint8_t)> onWriteCallback);
private:
    // Updated signature with connInfo parameter
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo& connInfo) override;
    std::function<void(uint8_t*, uint8_t)> onWriteCallback = nullptr;
};

extern BLEMidiServerClass BLEMidiServer;
#endif