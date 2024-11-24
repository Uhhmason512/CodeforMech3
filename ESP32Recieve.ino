#define ESP_RX 17 // RX pin on ESP32 (D7)
#define ESP_TX 16 // TX pin on ESP32 (D6)

int testVar = HIGH;


HardwareSerial SerialFromArduino(2); // Use UART2

void setup() {
    delay(2000);
    Serial.begin(115200); // Debugging via USB Serial
    Serial.println("Starting ESP32 Setup...");
    SerialFromArduino.begin(9600, SERIAL_8N1, ESP_RX, ESP_TX);
    Serial.println("ESP32 Ready to Receive Data from Arduino");
}


void loop() {
  delay(10);
   testVar = SerialFromArduino.available();
    Serial.println(testVar);
    if (SerialFromArduino.available()) {
        String receivedData = SerialFromArduino.readStringUntil('\n');
        Serial.println("Received: " + receivedData);
    }
}