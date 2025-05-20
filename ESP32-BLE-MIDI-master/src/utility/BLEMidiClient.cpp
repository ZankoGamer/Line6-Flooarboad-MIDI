#include "BLEMidiClient.h"

void BLEMidiClientClass::begin(const std::string deviceName)
{
    BLEMidi::begin(deviceName);
}

int BLEMidiClientClass::scan()
{
    debug.println("Beginning scan...");
    pBLEScan = NimBLEDevice::getScan();
    if(pBLEScan == nullptr)
        return 0;
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->clearResults();
    foundMidiDevices.clear();
    
    // Note: using time in milliseconds now (3000 ms = 3 seconds)
    NimBLEScanResults foundDevices = pBLEScan->getResults(3000, false);
    debug.printf("Found %d BLE device(s)\n", foundDevices.getCount());
    for(int i = 0; i < foundDevices.getCount(); i++) {
        const NimBLEAdvertisedDevice* device = foundDevices.getDevice(i);
        auto deviceStr = "name = \"" + device->getName() + "\", address = "  + device->getAddress().toString();
        if (device->haveServiceUUID() && device->isAdvertisingService(NimBLEUUID(MIDI_SERVICE_UUID))) {
            debug.println((" - BLE MIDI device : " + deviceStr).c_str());
            foundMidiDevices.push_back(*device);
        }
        else {
            debug.println((" - Other type of BLE device : " + deviceStr).c_str());
        }
        debug.printf("Total of BLE MIDI devices : %d\n", foundMidiDevices.size());
    }
    return foundMidiDevices.size();
}

NimBLEAdvertisedDevice* BLEMidiClientClass::getScannedDevice(uint32_t deviceIndex)
{
    if(deviceIndex >= foundMidiDevices.size()) {
        debug.println("Scanned device not found because requested index is greater than the devices list");
        return nullptr;
    }
    // Return a pointer to the stored device (note: it remains valid until next scan)
    return &foundMidiDevices.at(deviceIndex);
}

bool BLEMidiClientClass::connect(uint32_t deviceIndex)
{
    debug.printf("Connecting to device number %d\n", deviceIndex);
    if(deviceIndex >= foundMidiDevices.size()) {
        debug.println("Cannot connect: device index is greater than the size of the MIDI devices list.");
        return false;
    }
    // Create a new device object from the stored device
    NimBLEAdvertisedDevice* device = new NimBLEAdvertisedDevice(foundMidiDevices.at(deviceIndex));
    if(device == nullptr)
        return false;
    debug.printf("Address of the device : %s\n", device->getAddress().toString().c_str());
    NimBLEClient* pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallbacks(connected, onConnectCallback, onDisconnectCallback));
    debug.println("pClient->connect()");
    if(!pClient->connect(device))
        return false;
    debug.println("pClient->getService()");
    NimBLERemoteService* pRemoteService = pClient->getService(MIDI_SERVICE_UUID.c_str());
    if(pRemoteService == nullptr) {
        debug.println("Couldn't find remote service");
        return false;
    }
    debug.println("pRemoteService->getCharacteristic()");
    pRemoteCharacteristic = pRemoteService->getCharacteristic(MIDI_CHARACTERISTIC_UUID.c_str());
    if(pRemoteCharacteristic == nullptr) {
        debug.println("Couldn't find remote characteristic");
        return false;
    }
    debug.println("Registering characteristic callback");
    if(pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->subscribe(true, [](NimBLERemoteCharacteristic* pRemoteChar, uint8_t* pData, size_t length, bool isNotify) {
            BLEMidiClient.receivePacket(pData, length); // Call the member function of the global instance.
            vTaskDelay(0);  // Allow time for the IDLE task (see esp_task_wdt_reset_watchdog note)
        });
    }
    connected = true;
    return true;
}

void BLEMidiClientClass::setOnConnectCallback(void (*const onConnectCallback)())
{
    this->onConnectCallback = onConnectCallback;
}
void BLEMidiClientClass::setOnDisconnectCallback(void (*const onDisconnectCallback)())
{
    this->onDisconnectCallback = onDisconnectCallback;
}

void BLEMidiClientClass::sendPacket(uint8_t *packet, uint8_t packetSize)
{
    if(!connected)
        return;
    pRemoteCharacteristic->writeValue(packet, packetSize, false);
}

// --- ClientCallbacks Implementation ---

void ClientCallbacks::onConnect(NimBLEClient* pClient)
{
    connected = true;
    if(onConnectCallback != nullptr)
        onConnectCallback();
}

void ClientCallbacks::onDisconnect(NimBLEClient* pClient, int reason)
{
    connected = false;
    if(onDisconnectCallback != nullptr)
        onDisconnectCallback();
}

BLEMidiClientClass BLEMidiClient;
