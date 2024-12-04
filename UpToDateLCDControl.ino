#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set up LCD (20x4)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Pin Definitions
const int clkPin = 2;          // CLK pin on rotary encoder
const int dtPin = 3;           // DT pin on rotary encoder
const int swPin = 4;           // SW (button) pin on rotary encoder
const int motorControlPin = 7; // DC motor control pin (PWM) for setting RPM
const int relayPin = 8;        // Pin connected to the solid-state relay
const int tempPin = A0;        // Temperature probe analog input for EI-1034
const int phPin = A1;          // pH probe analog input
const int motorRpmPin = A6;    // Analog pin A6 to read RPM feedback

// Variables for Menu Navigation
int lastClkState;
int menuIndex = 0;
String menuItems[] = {"Heater", "Motor RPM", "Data", "PH Assembly"};
int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);
bool inDataPage = false;       // Flag to indicate if we're in the Data page

// Variables for Heater Control
bool adjustingHeaterTemp = false;
float desiredTemp = 40.0;      // Desired temperature in Celsius
bool heaterOn = false;         // Heater state

// Variables for Motor Control
bool adjustingMotorRPM = false;
int motorRPM = 0;              // Current motor RPM
int desiredRPM = 0;            // Desired motor RPM set by the user

void setup() {
    pinMode(clkPin, INPUT);
    pinMode(dtPin, INPUT);
    pinMode(swPin, INPUT_PULLUP);
    pinMode(motorControlPin, OUTPUT);
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW); // Ensure relay is off initially
    lcd.begin(20, 4);
    lcd.backlight();
    displayMenu();

    lastClkState = digitalRead(clkPin);
    Serial.begin(9600); // For minimal debugging purposes
}

unsigned long lastRefreshTime = 0; // Tracks the last refresh time for the data page
const unsigned long refreshInterval = 1000; // Refresh every 1 second (1000 ms)

void loop() {
    int clkState = digitalRead(clkPin); // Read the current state of CLK

    // Refresh Data Page every second if in Data Page
    if (inDataPage) {
        unsigned long currentTime = millis();
        if (currentTime - lastRefreshTime >= refreshInterval) {
            lastRefreshTime = currentTime; // Update last refresh time
            displayDataPage(); // Refresh the data page
        }
    }

    // Check for rotation
    if (clkState != lastClkState) { // Only process if the CLK state has changed
        delay(5); // Delay to stabilize the signal

        int dtState = digitalRead(dtPin); // Read the state of DT

        if (adjustingMotorRPM) {
            // Adjust motor RPM
            if (dtState != clkState) { 
                desiredRPM += 10; 
            } else {
                desiredRPM -= 10; 
            }
            desiredRPM = constrain(desiredRPM, 0, 150); 
            updateMotorRPMDisplay(); 
        } else if (adjustingHeaterTemp) {
            // Adjust heater temperature
            if (dtState != clkState) {
                desiredTemp += 0.1; // Smaller increments for precision
            } else {
                desiredTemp -= 0.1;
            }
            desiredTemp = constrain(desiredTemp, 30.0, 50.0); // Updated range
            updateHeaterTempDisplay();
        } else {
            // Navigate main menu
            if (dtState != clkState) {
                menuIndex = (menuIndex + 1) % menuSize; 
            } else {
                menuIndex = (menuIndex - 1 + menuSize) % menuSize; 
            }
            displayMenu(); 
        }
    }
    lastClkState = clkState; // Update the last known state of CLK

    // Check for button press
    if (digitalRead(swPin) == LOW) {
        delay(50); // Debounce delay
        if (digitalRead(swPin) == LOW) {
            if (inDataPage) {
                inDataPage = false; // Exit data page on button press
                displayMenu();
            } else if (menuItems[menuIndex] == "Motor RPM") {
                if (!adjustingMotorRPM) {
                    adjustingMotorRPM = true;
                    desiredRPM = motorRPM;
                    updateMotorRPMDisplay();
                } else {
                    motorRPM = desiredRPM;
                    analogWrite(motorControlPin, map(motorRPM, 0, 150, 0, 255));
                    adjustingMotorRPM = false;
                    displayMenu();
                }
            } else if (menuItems[menuIndex] == "Heater") {
                if (!adjustingHeaterTemp) {
                    adjustingHeaterTemp = true;
                    updateHeaterTempDisplay();
                } else {
                    adjustingHeaterTemp = false;
                    displayMenu();
                }
            } else if (menuItems[menuIndex] == "Data") {
                inDataPage = true;
                displayDataPage();
                lastRefreshTime = millis(); // Reset refresh timer when entering Data Page
            } else {
                selectMenuItem();
            }
            while (digitalRead(swPin) == LOW); // Wait for button release
        }
    }

    // Bang-bang control for heater
    manageHeater();
}

void displayMenu() {
    lcd.clear();
    for (int i = 0; i < menuSize; i++) {
        lcd.setCursor(0, i);
        if (i == menuIndex) {
            lcd.print("> ");
        } else {
            lcd.print("  ");
        }
        lcd.print(menuItems[i]);
    }
}

void updateMotorRPMDisplay() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Adjust Motor RPM");
    lcd.setCursor(0, 1);
    lcd.print("Current RPM: ");
    lcd.print(motorRPM);
    lcd.setCursor(0, 2);
    lcd.print("New RPM: ");
    lcd.print(desiredRPM);
}

void updateHeaterTempDisplay() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Adjust Heater Temp");
    lcd.setCursor(0, 1);
    lcd.print("Desired Temp: ");
    lcd.print(desiredTemp, 1);
    lcd.print(" C");
}

void displayDataPage() {
    lcd.clear();

    // Read temperature from EI-1034 temperature probe
    int sensorValue = analogRead(tempPin);
    float voltage = sensorValue * (5.0 / 1023.0);
    float temperatureC = (55.56 * voltage) + 255.37 - 273.15;

    // Display data
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperatureC, 1);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("> Back");
}

void manageHeater() {
    int sensorValue = analogRead(tempPin);
    float voltage = sensorValue * (5.0 / 1023.0);
    float currentTemp = (55.56 * voltage) + 255.37 - 273.15;

    if (currentTemp < desiredTemp - 0.6) {
        digitalWrite(relayPin, HIGH); // Turn on heater
        heaterOn = true;
    } else if (currentTemp > desiredTemp + 0.6) {
        digitalWrite(relayPin, LOW); // Turn off heater
        heaterOn = false;
    }
}

void selectMenuItem() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Selected: ");
    lcd.setCursor(0, 1);
    lcd.print(menuItems[menuIndex]);
    delay(1000);
    displayMenu();
}
