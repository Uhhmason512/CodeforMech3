const int tempPin = A0; // Analog pin connected to the EI-1034 signal output
float voltage = 0;      // Variable to store the voltage reading
float temperatureF = 0; // Variable to store the calculated temperature in Fahrenheit

void setup() {
    Serial.begin(9600); // Initialize serial communication for debugging
}

void loop() {
    // Read the analog voltage
    int sensorValue = analogRead(tempPin); // Read the analog input (0-1023)
    voltage = sensorValue * (5.0 / 1023.0); // Convert to voltage (0-5V)

    // Convert voltage to temperature in Fahrenheit
    temperatureF = voltage / 0.01; // 0.01V per °F

    // Print the results
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.print(" V, Temperature: ");
    Serial.print(temperatureF);
    Serial.println(" °F");

    delay(1000); // Wait for a second before the next reading
}



