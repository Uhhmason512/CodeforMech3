void setup() {
    Serial.begin(9600); // Initialize serial communication
}

void loop() {
    Serial.println("Hello from Arduino!");
    delay(1000); // Send data every second
}
