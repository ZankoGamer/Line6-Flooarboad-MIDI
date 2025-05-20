#include "BLEMidiServer.h"

void BLEMidiServerClass::begin(const std::string deviceName) {
    // Initialize base BLE MIDI functionality
    BLEMidi::begin(deviceName);

    // Initialize NimBLE and set device name
    NimBLEDevice::init(deviceName);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Optional: maximize TX power

    // Configure security: allow pairing/bonding without IO
    NimBLEDevice::setSecurityAuth(true, true, true);                // bonding, MITM, SC
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);      // no input/output

    // Create BLE server and assign callbacks
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(this);

    // Create MIDI service and characteristic
    NimBLEService *pService = pServer->createService(MIDI_SERVICE_UUID);
    uint16_t charProps =
        NIMBLE_PROPERTY::READ        |  // readable
        NIMBLE_PROPERTY::WRITE       |  // writable
        NIMBLE_PROPERTY::WRITE_NR    |  // non-registered write
        NIMBLE_PROPERTY::NOTIFY;         // notifications

    pCharacteristic = pService->createCharacteristic(
        MIDI_CHARACTERISTIC_UUID,
        charProps
    );
    pCharacteristic->setCallbacks(new CharacteristicCallback(
        [this](uint8_t *data, uint8_t size) { this->receivePacket(data, size); }
    ));
    pService->start();

    // Build advertising data and scan response
    NimBLEAdvertising *pAdvertising = pServer->getAdvertising();

    // Advertising packet: flags, appearance, and service UUID
    NimBLEAdvertisementData advData;
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    advData.setAppearance(640);                      // #define BLE_APPEARANCE_GENERIC_MEDIA_PLAYER   640
    advData.addServiceUUID(MIDI_SERVICE_UUID);          // Complete 128-bit list
    pAdvertising->setAdvertisementData(advData);

    // Scan response: advertise full device name
    NimBLEAdvertisementData scanData;
    scanData.setName(deviceName);
    pAdvertising->setScanResponseData(scanData);

    // Start advertising
    if (pAdvertising->start()) {
        Serial.println("BLE MIDI Advertising started");
    } else {
        Serial.println("BLE MIDI Advertising failed");
    }
}

void BLEMidiServerClass::setOnConnectCallback(void (*const onConnectCallback)()) {
    this->onConnectCallback = onConnectCallback;
}

void BLEMidiServerClass::setOnDisconnectCallback(void (*const onDisconnectCallback)()) {
    this->onDisconnectCallback = onDisconnectCallback;
}

void BLEMidiServerClass::sendPacket(uint8_t *packet, uint8_t packetSize) {
    if (!connected) return;
    pCharacteristic->setValue(packet, packetSize);
    pCharacteristic->notify();
}

void BLEMidiServerClass::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    connected = true;
    if (onConnectCallback) onConnectCallback();
}

void BLEMidiServerClass::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    connected = false;
    if (onDisconnectCallback) onDisconnectCallback();
    // Restart advertising manually
    pServer->getAdvertising()->start();
}

CharacteristicCallback::CharacteristicCallback(std::function<void(uint8_t*, uint8_t)> onWriteCallback)
    : onWriteCallback(onWriteCallback) {}

void CharacteristicCallback::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo& connInfo) {
    std::string rxValue = pCharacteristic->getValue();
    if (!rxValue.empty() && onWriteCallback) {
        onWriteCallback((uint8_t*)rxValue.c_str(), static_cast<uint8_t>(rxValue.length()));
    }
}

BLEMidiServerClass BLEMidiServer;
