#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ezButton.h>

// =========================== LCD SETUP ===========================
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ========================== PIN DEFINITIONS ==========================
// Original Pins
const int clkPin = 2;          // CLK pin on rotary encoder
const int dtPin = 3;           // DT pin on rotary encoder
const int swPin = 4;           // SW (button) pin on rotary encoder
const int motorControlPin = 7; // Motor control pin (PWM) for RPM
const int relayPin = 8;        // Relay pin for heater control
const int tempPin = A0;        // Temperature probe analog input (EI-1034)
const int phPin = A1;          // pH probe analog input
const int motorRpmPin = A6;    // RPM feedback input (analog)

// New Pins for pH Assembly
const int stepPin = 9;         // Stepper motor step pin
const int dirPin = 10;         // Stepper motor direction pin
const int limitSwitchPin = 11; // Limit switch pin for pH assembly

// Encoder Pins for DC Motor
const int encoderAPin = 12;    // Encoder A signal
const int encoderBPin = 13;    // Encoder B signal


// =========================== VARIABLES ===========================
// Original Variables
int lastClkState;                      
int menuIndex = 0;                     
String menuItems[] = {"Heater", "Motor RPM", "Data", "PH Assembly"}; 
int menuSize = sizeof(menuItems) / sizeof(menuItems[0]); 
bool inDataPage = false;               
bool adjustingHeaterTemp = false;      
float desiredTemp = 40.0;              
bool heaterOn = false;                 
bool adjustingMotorRPM = false;        
int motorRPM = 0;                      
int desiredRPM = 0;                    
unsigned long lastRefreshTime = 0;     
const unsigned long refreshInterval = 1000; 

// New Variables for pH Assembly
String phSubMenuItems[] = {"Run", "Set Run Time"};
int phSubMenuIndex = 0;
int phSubMenuSize = sizeof(phSubMenuItems) / sizeof(phSubMenuItems[0]);
bool inPhSubMenu = false;
ezButton limitSwitch(limitSwitchPin);

// Encoder Variables
volatile int encoderPulses = 0;  // Pulse count from encoder
unsigned long lastRpmCalcTime = 0;
const int encoderPulsesPerRevolution = 20; // Adjust to match your encoder

// =========================== SETUP FUNCTION ===========================
void setup() {
    // Original Pin Configurations
    pinMode(clkPin, INPUT);
    pinMode(dtPin, INPUT);
    pinMode(swPin, INPUT_PULLUP);
    pinMode(motorControlPin, OUTPUT);
    pinMode(relayPin, OUTPUT);

    // pH Assembly Pin Configurations
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(limitSwitchPin, INPUT_PULLUP);
    limitSwitch.setDebounceTime(50);

    // Ensure relay is OFF initially
    digitalWrite(relayPin, LOW);

    // Encoder Setup
    pinMode(encoderAPin, INPUT);
    pinMode(encoderBPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(encoderAPin), readEncoder, RISING);

    // Initialize LCD and display menu
    lcd.begin(20, 4);
    lcd.backlight();
    displayMenu();

    // Initialize rotary encoder state
    lastClkState = digitalRead(clkPin);

    // Start Serial Communication
    Serial.begin(9600);
}

// =========================== LOOP FUNCTION ===========================
void loop() {
    int clkState = digitalRead(clkPin); // Read rotary encoder CLK pin

    // Refresh Data Page if active
    if (inDataPage) {
        unsigned long currentTime = millis();
        if (currentTime - lastRefreshTime >= refreshInterval) {
            lastRefreshTime = currentTime;
            displayDataPage();
        }
    }

    // Handle Rotary Encoder Rotation
    if (clkState != lastClkState) {
        delay(5); // Stabilization delay
        int dtState = digitalRead(dtPin);

        if (inPhSubMenu) {
            navigatePhSubMenu(dtState != clkState);
        } else if (adjustingMotorRPM) {
            if (dtState != clkState) desiredRPM += 10;
            else desiredRPM -= 10;
            desiredRPM = constrain(desiredRPM, 0, 150);
            updateMotorRPMDisplay();
        } else if (adjustingHeaterTemp) {
            if (dtState != clkState) desiredTemp += 0.1;
            else desiredTemp -= 0.1;
            desiredTemp = constrain(desiredTemp, 30.0, 50.0);
            updateHeaterTempDisplay();
        } else {
            if (dtState != clkState) menuIndex = (menuIndex + 1) % menuSize;
            else menuIndex = (menuIndex - 1 + menuSize) % menuSize;
            displayMenu();
        }

        lastClkState = clkState;
    }

    // Handle Button Press
    if (digitalRead(swPin) == LOW) {
        delay(50);
        if (digitalRead(swPin) == LOW) {
            handleButtonPress();
            while (digitalRead(swPin) == LOW);
        }
    }

    // Manage Heater
    manageHeater();

    // Calculate and Control Motor RPM
    controlMotorRPM();
}

// ======================= RPM CALCULATION =======================
void readEncoder() {
    int bState = digitalRead(encoderBPin);
    encoderPulses += (bState == HIGH) ? 1 : -1; // Increment or decrement pulse count
}

void controlMotorRPM() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastRpmCalcTime >= 1000) { // Every second
        motorRPM = (encoderPulses * 60) / encoderPulsesPerRevolution;
        encoderPulses = 0; // Reset pulse count
        lastRpmCalcTime = currentMillis;

        // Debug Output
        Serial.print("Measured RPM: ");
        Serial.println(motorRPM);

        // Adjust motor speed (simple proportional control)
        int rpmError = desiredRPM - motorRPM;
        int pwmValue = constrain(rpmError * 2, 0, 255); // Proportional constant = 2
        analogWrite(motorControlPin, pwmValue);

        // Debug Output
        Serial.print("PWM Value: ");
        Serial.println(pwmValue);
    }
}


// ======================= BUTTON PRESS HANDLING =======================
void handleButtonPress() {
    if (inDataPage) {
        inDataPage = false;
        displayMenu();
    } else if (menuItems[menuIndex] == "PH Assembly") {
        if (!inPhSubMenu) {
            inPhSubMenu = true;
            displayPhSubMenu();
        } else {
            if (phSubMenuItems[phSubMenuIndex] == "Run") {
                inPhSubMenu = false;
                runPhAssembly();
            } else if (phSubMenuItems[phSubMenuIndex] == "Set Run Time") {
                inPhSubMenu = false;
                setRunTime();
            }
        }
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
        lastRefreshTime = millis();
    } else {
        selectMenuItem();
    }
}

// ======================= MENU NAVIGATION =======================
void displayMenu() {
    lcd.clear();
    for (int i = 0; i < menuSize; i++) {
        lcd.setCursor(0, i);
        lcd.print(i == menuIndex ? "> " : "  ");
        lcd.print(menuItems[i]);
    }
}

void displayPhSubMenu() {
    lcd.clear();
    for (int i = 0; i < phSubMenuSize; i++) {
        lcd.setCursor(0, i);
        lcd.print(i == phSubMenuIndex ? "> " : "  ");
        lcd.print(phSubMenuItems[i]);
    }
}

void navigatePhSubMenu(bool clockwise) {
    if (clockwise) phSubMenuIndex = (phSubMenuIndex + 1) % phSubMenuSize;
    else phSubMenuIndex = (phSubMenuIndex - 1 + phSubMenuSize) % phSubMenuSize;
    displayPhSubMenu();
}

// ======================= pH ASSEMBLY =======================
void runPhAssembly() {
    Serial.println("PH is running");
    lcd.clear();
    lcd.print("PH is running...");
    delay(1000);

    // Set timeout (e.g., 10 seconds)
    unsigned long startTime = millis();

    // Forward Movement with Timeout
    digitalWrite(dirPin, HIGH);
    while (!limitSwitch.getState() && (millis() - startTime < 10000)) {
        for (int x = 0; x < 100; x++) {
            digitalWrite(stepPin, HIGH);
            delayMicroseconds(500);
            digitalWrite(stepPin, LOW);
            delayMicroseconds(500);
        }
        limitSwitch.loop();
    }

    delay(1000);

    // Reset timeout
    startTime = millis();

    // Reverse Movement with Timeout
    digitalWrite(dirPin, LOW);
    while (limitSwitch.getState() && (millis() - startTime < 10000)) {
        for (int x = 0; x < 100; x++) {
            digitalWrite(stepPin, HIGH);
            delayMicroseconds(500);
            digitalWrite(stepPin, LOW);
            delayMicroseconds(500);
        }
        limitSwitch.loop();
    }

    Serial.println("PH assembly completed (Test)");
    lcd.clear();
    lcd.print("PH assembly done (Test)");
    delay(2000);
    displayMenu();
}


void setRunTime() {
    Serial.println("Set Run Time selected");
    lcd.clear();
    lcd.print("Set Run Time...");
    delay(2000);
    displayMenu();
}

void selectMenuItem() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Selected: ");
    lcd.setCursor(0, 1);
    lcd.print(menuItems[menuIndex]);
    delay(1000); // Pause briefly to display the selection
    displayMenu(); // Return to the menu
}

// ======================= HEATER CONTROL =======================
void manageHeater() {
    int sensorValue = analogRead(tempPin);
    float voltage = sensorValue * (5.0 / 1023.0);
    float currentTemp = (55.56 * voltage) + 255.37 - 273.15;

    if (currentTemp < desiredTemp - 0.6) {
        digitalWrite(relayPin, HIGH);
        heaterOn = true;
    } else if (currentTemp > desiredTemp + 0.6) {
        digitalWrite(relayPin, LOW);
        heaterOn = false;
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
    int tempSensorValue = analogRead(tempPin);
    float tempVoltage = tempSensorValue * (5.0 / 1023.0);
    float temperatureC = (55.56 * tempVoltage) + 255.37 - 273.15;

    int phSensorValue = analogRead(phPin);
    float phValue = (phSensorValue * 5.0) / 1023.0;

    int motorFeedbackValue = analogRead(motorRpmPin);
    motorRPM = map(motorFeedbackValue, 0, 1023, 0, 150);

    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperatureC, 1);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("pH: ");
    lcd.print(phValue, 2);

    lcd.setCursor(0, 2);
    lcd.print("Motor RPM: ");
    lcd.print(motorRPM);

    lcd.setCursor(0, 3);
    lcd.print("> Back");
}
