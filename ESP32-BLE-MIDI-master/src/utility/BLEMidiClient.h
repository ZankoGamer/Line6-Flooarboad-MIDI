#ifndef BLE_MIDI_CLIENT_H
#define BLE_MIDI_CLIENT_H

#include <vector>
#include "BLEMidiBase.h"

// Note: we now use NimBLE types for scan results and advertised devices.
class BLEMidiClientClass : public BLEMidi {
public:

    /// Initializes the BLEMidiClient
    void begin(const std::string deviceName) override;

    /// Begins a scan, and returns the number of MIDI devices found.
    int scan();

    /// Returns the nth scanned MIDI device, or nullptr in case of an error.
    /// Do not use the returned value if you perform another scan later, because it will be cleared.
    NimBLEAdvertisedDevice* getScannedDevice(uint32_t deviceIndex);

    /// Connects to the nth scanned MIDI device
    bool connect(uint32_t deviceIndex = 0);

    void setOnConnectCallback(void (*const onConnectCallback)());
    void setOnDisconnectCallback(void (*const onDisconnectCallback)());

private:
    /// This method is called by the base Midi class to send packets.
    virtual void sendPacket(uint8_t *packet, uint8_t packetSize) override;

    NimBLEScan *pBLEScan = nullptr;
    std::vector<NimBLEAdvertisedDevice> foundMidiDevices;
    NimBLERemoteCharacteristic* pRemoteCharacteristic;
    void (*onConnectCallback)() = nullptr;
    void (*onDisconnectCallback)() = nullptr;
};

/// Callbacks for client connections and disconnections
class ClientCallbacks : public NimBLEClientCallbacks {
public:
    ClientCallbacks(
            bool& connected,
            void (* const onConnectCallback)() = nullptr, 
            void (* const onDisconnectCallback)() = nullptr
    );
private:
    void onConnect(NimBLEClient* pClient) override;
	void onDisconnect(NimBLEClient* pClient, int reason) override;


    bool& connected;
    void (* const onConnectCallback)();
    void (* const onDisconnectCallback)();
};

extern BLEMidiClientClass BLEMidiClient;

#endif
