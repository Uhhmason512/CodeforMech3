#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLE Service UUID
#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
// BLE Characteristic UUID
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-123456789abc"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

void setup() {
    Serial.begin(115200);

    // Initialize BLE
    BLEDevice::init("ESP32_BLE");
    pServer = BLEDevice::createServer();

    // Create BLE Service
    BLEService* pService = pServer->createService(SERVICE_UUID);

    // Create BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );

    // Set initial value for the characteristic
    pCharacteristic->setValue("Hello from ESP32!");
    
    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->start();
    Serial.println("BLE server started!");
}

void loop() {
    delay(1000);
}
