#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ezButton.h>

// =========================== LCD SETUP ===========================
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ========================== PIN DEFINITIONS ==========================
const int clkPin = 2;          // Rotary encoder CLK
const int dtPin = 3;           // Rotary encoder DT
const int swPin = 4;           // Rotary encoder button
const int motorControlPin = 7; // Motor PWM control
const int relayPin = 8;        // Heater relay control
const int tempPin = A0;        // Temperature sensor input
const int phPin = A1;          // pH probe input
const int motorRpmPin = A6;    // Motor RPM feedback input

// Pins for pH Assembly
const int stepPin = 9;         // Stepper motor step
const int dirPin = 10;         // Stepper motor direction
const int limitSwitchPin = 11; // Limit switch

// Encoder Pins for DC Motor
const int encoderAPin = 12;    // Encoder A signal
const int encoderBPin = 13;    // Encoder B signal

// =========================== VARIABLES ===========================
int lastClkState;
int menuIndex = 0;
String menuItems[] = {"Heater", "Motor RPM", "Data", "PH Assembly"};
int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);
bool inDataPage = false;
bool adjustingHeaterTemp = false;
float desiredTemp = 40.0;
bool heaterOn = false;
bool adjustingMotorRPM = false;
int motorRPM = 0;              // Desired RPM
int actualMotorRPM = 0;        // Measured RPM
unsigned long lastRefreshTime = 0;
const unsigned long refreshInterval = 1000;

// Timer Variables
const unsigned long dataSendInterval = 60000;
unsigned long lastDataSendTime = 0;

// pH Assembly Variables
String phSubMenuItems[] = {"Run", "Set Run Time"};
int phSubMenuIndex = 0;
int phSubMenuSize = sizeof(phSubMenuItems) / sizeof(phSubMenuItems[0]);
bool inPhSubMenu = false;
ezButton limitSwitch(limitSwitchPin);

// Encoder Variables
volatile int encoderPulses = 0;
unsigned long lastRpmCalcTime = 0;
const int encoderPulsesPerRevolution = 20;

// pH Run Time Variables
int phRunTimesPerHour = 6;
unsigned long phRunInterval = 0;
unsigned long lastPhRunTime = 0;

// =========================== SETUP FUNCTION ===========================
void setup() {
    pinMode(clkPin, INPUT);
    pinMode(dtPin, INPUT);
    pinMode(swPin, INPUT_PULLUP);
    pinMode(motorControlPin, OUTPUT);
    pinMode(relayPin, OUTPUT);

    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(limitSwitchPin, INPUT_PULLUP);
    limitSwitch.setDebounceTime(50);

    digitalWrite(relayPin, LOW);

    pinMode(encoderAPin, INPUT);
    pinMode(encoderBPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(encoderAPin), readEncoder, RISING);

    lcd.begin(20, 4);
    lcd.backlight();
    displayMenu();

    lastClkState = digitalRead(clkPin);
    updatePHRunInterval();

    Serial.begin(9600);
}

// =========================== LOOP FUNCTION ===========================
void loop() {
    int clkState = digitalRead(clkPin);

    if (inDataPage) {
        unsigned long currentTime = millis();
        if (currentTime - lastRefreshTime >= refreshInterval) {
            lastRefreshTime = currentTime;
            displayDataPage();
        }
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastDataSendTime >= dataSendInterval) {
        lastDataSendTime = currentMillis;
        sendDataToESP32();
    }

    if (currentMillis - lastPhRunTime >= phRunInterval) {
        lastPhRunTime = currentMillis;
        runPhAssembly();
    }

    if (clkState != lastClkState) {
        delay(5);
        int dtState = digitalRead(dtPin);

        if (inPhSubMenu) {
            navigatePhSubMenu(dtState != clkState);
        } else if (adjustingMotorRPM) {
            motorRPM += (dtState != clkState) ? 10 : -10;
            motorRPM = constrain(motorRPM, 0, 150);
            updateMotorRPMDisplay();
        } else if (adjustingHeaterTemp) {
            desiredTemp += (dtState != clkState) ? 0.1 : -0.1;
            desiredTemp = constrain(desiredTemp, 30.0, 50.0);
            updateHeaterTempDisplay();
        } else {
            menuIndex = (dtState != clkState) ? (menuIndex + 1) % menuSize : (menuIndex - 1 + menuSize) % menuSize;
            displayMenu();
        }

        lastClkState = clkState;
    }

    if (digitalRead(swPin) == LOW) {
        delay(50);
        if (digitalRead(swPin) == LOW) {
            handleButtonPress();
            while (digitalRead(swPin) == LOW);
        }
    }

    manageHeater();
    controlMotorRPM();
}

// ======================= RPM CALCULATION =======================
void readEncoder() {
    int bState = digitalRead(encoderBPin);
    encoderPulses += (bState == HIGH) ? 1 : -1;
}

void controlMotorRPM() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastRpmCalcTime >= 1000) { // Every second
        actualMotorRPM = (encoderPulses * 60) / encoderPulsesPerRevolution;
        encoderPulses = 0;
        lastRpmCalcTime = currentMillis;

        Serial.print("Measured RPM: ");
        Serial.println(actualMotorRPM);

        int rpmError = motorRPM - actualMotorRPM;
        int pwmValue = constrain(rpmError * 2, 0, 255); // Proportional control
        analogWrite(motorControlPin, pwmValue);

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
        } else if (phSubMenuItems[phSubMenuIndex] == "Run") {
            runPhAssembly();
        } else if (phSubMenuItems[phSubMenuIndex] == "Set Run Time") {
            setRunTime();
        }
    } else if (menuItems[menuIndex] == "Motor RPM") {
        adjustingMotorRPM = !adjustingMotorRPM;
        if (!adjustingMotorRPM) {
            displayMenu();
        } else {
            updateMotorRPMDisplay();
        }
    } else if (menuItems[menuIndex] == "Heater") {
        adjustingHeaterTemp = !adjustingHeaterTemp;
        if (!adjustingHeaterTemp) {
            displayMenu();
        } else {
            updateHeaterTempDisplay();
        }
    } else if (menuItems[menuIndex] == "Data") {
        inDataPage = true;
        displayDataPage();
    }
}

// ======================= MENU NAVIGATION =======================
void displayMenu() {
    lcd.clear();
    for (int i = 0; i < menuSize; i++) {
        lcd.setCursor(0, i);
        lcd.print((i == menuIndex) ? "> " : "  ");
        lcd.print(menuItems[i]);
    }
}

void updateMotorRPMDisplay() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Adjust Motor RPM");
    lcd.setCursor(0, 1);
    lcd.print("Current RPM: ");
    lcd.print(actualMotorRPM);
    lcd.setCursor(0, 2);
    lcd.print("Desired RPM: ");
    lcd.print(motorRPM);
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
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ezButton.h>

// =========================== LCD SETUP ===========================
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ========================== PIN DEFINITIONS ==========================
const int clkPin = 2;          // Rotary encoder CLK
const int dtPin = 3;           // Rotary encoder DT
const int swPin = 4;           // Rotary encoder button
const int motorControlPin = 7; // Motor PWM control
const int relayPin = 8;        // Heater relay control
const int tempPin = A0;        // Temperature sensor input
const int phPin = A1;          // pH probe input
const int motorRpmPin = A6;    // Motor RPM feedback input

// Pins for pH Assembly
const int stepPin = 9;         // Stepper motor step
const int dirPin = 10;         // Stepper motor direction
const int limitSwitchPin = 11; // Limit switch

// Encoder Pins for DC Motor
const int encoderAPin = 12;    // Encoder A signal
const int encoderBPin = 13;    // Encoder B signal

// =========================== VARIABLES ===========================
int lastClkState;
int menuIndex = 0;
String menuItems[] = {"Heater", "Motor RPM", "Data", "PH Assembly"};
int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);
bool inDataPage = false;
bool adjustingHeaterTemp = false;
float desiredTemp = 40.0;
bool heaterOn = false;
bool adjustingMotorRPM = false;
int motorRPM = 0;              // Desired RPM
int actualMotorRPM = 0;        // Measured RPM
unsigned long lastRefreshTime = 0;
const unsigned long refreshInterval = 1000;

// Timer Variables
const unsigned long dataSendInterval = 60000;
unsigned long lastDataSendTime = 0;

// pH Assembly Variables
String phSubMenuItems[] = {"Run", "Set Run Time"};
int phSubMenuIndex = 0;
int phSubMenuSize = sizeof(phSubMenuItems) / sizeof(phSubMenuItems[0]);
bool inPhSubMenu = false;
ezButton limitSwitch(limitSwitchPin);

// Encoder Variables
volatile int encoderPulses = 0;
unsigned long lastRpmCalcTime = 0;
const int encoderPulsesPerRevolution = 20;

// pH Run Time Variables
int phRunTimesPerHour = 6;
unsigned long phRunInterval = 0;
unsigned long lastPhRunTime = 0;

// =========================== SETUP FUNCTION ===========================
void setup() {
    pinMode(clkPin, INPUT);
    pinMode(dtPin, INPUT);
    pinMode(swPin, INPUT_PULLUP);
    pinMode(motorControlPin, OUTPUT);
    pinMode(relayPin, OUTPUT);

    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(limitSwitchPin, INPUT_PULLUP);
    limitSwitch.setDebounceTime(50);

    digitalWrite(relayPin, LOW);

    pinMode(encoderAPin, INPUT);
    pinMode(encoderBPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(encoderAPin), readEncoder, RISING);

    lcd.begin(20, 4);
    lcd.backlight();
    displayMenu();

    lastClkState = digitalRead(clkPin);
    updatePHRunInterval();

    Serial.begin(9600);
}

// =========================== LOOP FUNCTION ===========================
void loop() {
    int clkState = digitalRead(clkPin);

    if (inDataPage) {
        unsigned long currentTime = millis();
        if (currentTime - lastRefreshTime >= refreshInterval) {
            lastRefreshTime = currentTime;
            displayDataPage();
        }
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastDataSendTime >= dataSendInterval) {
        lastDataSendTime = currentMillis;
        sendDataToESP32();
    }

    if (currentMillis - lastPhRunTime >= phRunInterval) {
        lastPhRunTime = currentMillis;
        runPhAssembly();
    }

    if (clkState != lastClkState) {
        delay(5);
        int dtState = digitalRead(dtPin);

        if (inPhSubMenu) {
            navigatePhSubMenu(dtState != clkState);
        } else if (adjustingMotorRPM) {
            motorRPM += (dtState != clkState) ? 10 : -10;
            motorRPM = constrain(motorRPM, 0, 150);
            updateMotorRPMDisplay();
        } else if (adjustingHeaterTemp) {
            desiredTemp += (dtState != clkState) ? 0.1 : -0.1;
            desiredTemp = constrain(desiredTemp, 30.0, 50.0);
            updateHeaterTempDisplay();
        } else {
            menuIndex = (dtState != clkState) ? (menuIndex + 1) % menuSize : (menuIndex - 1 + menuSize) % menuSize;
            displayMenu();
        }

        lastClkState = clkState;
    }

    if (digitalRead(swPin) == LOW) {
        delay(50);
        if (digitalRead(swPin) == LOW) {
            handleButtonPress();
            while (digitalRead(swPin) == LOW);
        }
    }

    manageHeater();
    controlMotorRPM();
}

// ======================= RPM CALCULATION =======================
void readEncoder() {
    int bState = digitalRead(encoderBPin);
    encoderPulses += (bState == HIGH) ? 1 : -1;
}

void controlMotorRPM() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastRpmCalcTime >= 1000) { // Every second
        actualMotorRPM = (encoderPulses * 60) / encoderPulsesPerRevolution;
        encoderPulses = 0;
        lastRpmCalcTime = currentMillis;

        Serial.print("Measured RPM: ");
        Serial.println(actualMotorRPM);

        int rpmError = motorRPM - actualMotorRPM;
        int pwmValue = constrain(rpmError * 2, 0, 255); // Proportional control
        analogWrite(motorControlPin, pwmValue);

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
        } else if (phSubMenuItems[phSubMenuIndex] == "Run") {
            runPhAssembly();
        } else if (phSubMenuItems[phSubMenuIndex] == "Set Run Time") {
            setRunTime();
        }
    } else if (menuItems[menuIndex] == "Motor RPM") {
        adjustingMotorRPM = !adjustingMotorRPM;
        if (!adjustingMotorRPM) {
            displayMenu();
        } else {
            updateMotorRPMDisplay();
        }
    } else if (menuItems[menuIndex] == "Heater") {
        adjustingHeaterTemp = !adjustingHeaterTemp;
        if (!adjustingHeaterTemp) {
            displayMenu();
        } else {
            updateHeaterTempDisplay();
        }
    } else if (menuItems[menuIndex] == "Data") {
        inDataPage = true;
        displayDataPage();
    }
}

// ======================= MENU NAVIGATION =======================
void displayMenu() {
    lcd.clear();
    for (int i = 0; i < menuSize; i++) {
        lcd.setCursor(0, i);
        lcd.print((i == menuIndex) ? "> " : "  ");
        lcd.print(menuItems[i]);
    }
}

void updateMotorRPMDisplay() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Adjust Motor RPM");
    lcd.setCursor(0, 1);
    lcd.print("Current RPM: ");
    lcd.print(actualMotorRPM);
    lcd.setCursor(0, 2);
    lcd.print("Desired RPM: ");
    lcd.print(motorRPM);
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
