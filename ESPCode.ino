#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UART pins
#define ESP_RX 17 // ESP32 RX pin connected to Arduino TX
#define ESP_TX 16 // ESP32 TX pin connected to Arduino RX

// BLE Server and Characteristic
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;

// UART Communication
HardwareSerial SerialFromArduino(1); // Use UART1 for serial communication

// BLE Callback Class
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        deviceConnected = true;
        Serial.println("BLE Device Connected");
    }

    void onDisconnect(BLEServer *pServer) {
        deviceConnected = false;
        Serial.println("BLE Device Disconnected");
    }
};

void setup() {
    // Serial Monitor for debugging
    Serial.begin(9600);

    // UART communication setup
    SerialFromArduino.begin(9600, SERIAL_8N1, ESP_RX, ESP_TX);
    Serial.println("UART communication with Arduino initialized");

    // Initialize BLE
    BLEDevice::init("ESP32 Data Receiver");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create BLE Service and Characteristic
    BLEService *pService = pServer->createService(BLEUUID("12345678-1234-1234-1234-123456789012"));
    pCharacteristic = pService->createCharacteristic(
        BLEUUID("87654321-4321-4321-4321-210987654321"),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();

    // Start BLE Advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->start();
    Serial.println("Waiting for BLE client connection...");
}

void loop() {
    // Check for data from Arduino via UART
    if (SerialFromArduino.available()) {
        // Read the incoming data from Arduino
        String receivedData = SerialFromArduino.readStringUntil('\n');
        Serial.println("Received from Arduino: " + receivedData);

        // Transmit the data to the BLE client
        if (deviceConnected) {
            pCharacteristic->setValue(receivedData.c_str());
            pCharacteristic->notify();
            Serial.println("Data sent to BLE client: " + receivedData);
        }
    }

    delay(10); // Prevent excessive CPU usage
}
