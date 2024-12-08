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

// New Pins for pH Assembly
const int stepPin = 9;         // Stepper motor step pin
const int dirPin = 10;         // Stepper motor direction pin
const int limitSwitchPin = 11; // Limit switch pin for pH assembly

// Encoder Pins for DC Motor
const int encoderAPin = 12;    // Encoder A signal
const int encoderBPin = 13;    // Encoder B signal

const int methaneSensorPin = A2; // MP-4 sensor output connected to analog pin A2

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
int desiredRPM = 0;            // Desired RPM set by user
int actualMotorRPM = 0;        // Measured RPM from encoder
unsigned long lastRefreshTime = 0;
const unsigned long refreshInterval = 1000;

// New Variables for pH Assembly
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
int runTimesConstraintMin = 1;   
int runTimesConstraintMax = 15;  
bool resetPhRunInterval = true;

const float methaneThresholdVoltage = 2.0; // Threshold voltage to detect methane
bool methaneDetected = false; // Boolean to track methane detection status

const unsigned long dataSendInterval = 1800000; // 30 minutes
unsigned long lastDataSendTime = 0;

// =========================== FUNCTION PROTOTYPES ===========================
String getDataPage();
void sendDataToBLE(String data);


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
    pinMode(methaneSensorPin, INPUT); // Configure methane sensor pin as input
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
    if (currentMillis - lastPhRunTime >= phRunInterval) {
        lastPhRunTime = currentMillis;
        runPhAssembly();
    }

    if (currentMillis - lastDataSendTime >= dataSendInterval) {
        lastDataSendTime = currentMillis;
        String dataPage = getDataPage();
        Serial.println(dataPage);
    }


    if (clkState != lastClkState) {
        delay(5);
        int dtState = digitalRead(dtPin);

        if (inPhSubMenu) {
            navigatePhSubMenu(dtState != clkState);
        } else if (adjustingMotorRPM) {
            desiredRPM += (dtState != clkState) ? 10 : -10;
            desiredRPM = constrain(desiredRPM, 0, 150);
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

// ======================= FUNCTION IMPLEMENTATIONS =======================
String getDataPage() {
    // Temperature sensor
    int tempSensorValue = analogRead(tempPin);
    float tempVoltage = tempSensorValue * (5.0 / 1023.0);
    float temperatureC = (55.56 * tempVoltage) + 255.37 - 273.15;

    // pH sensor
    int phSensorValue = analogRead(phPin);
    float phValue = (phSensorValue * 5.0) / 1023.0;

    // Motor RPM calculation
    actualMotorRPM = (encoderPulses * 60) / encoderPulsesPerRevolution;

    // Methane detection
    int methaneSensorValue = analogRead(methaneSensorPin);
    float methaneVoltage = (methaneSensorValue / 1023.0) * 5.0;
    methaneDetected = methaneVoltage > methaneThresholdVoltage;

    String methaneStatus = methaneDetected ? "Detected" : "Not Detected";

    // Return combined data
    return "Temp: " + String(temperatureC, 1) + " C, pH: " + String(phValue, 2) + 
           ", Motor RPM: " + String(actualMotorRPM) + ", Methane: " + methaneStatus;
}



// ======================= RPM CALCULATION =======================
void readEncoder() {
    int bState = digitalRead(encoderBPin);
    encoderPulses += (bState == HIGH) ? 1 : -1;
}

void controlMotorRPM() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastRpmCalcTime >= 1000) {
        actualMotorRPM = (encoderPulses * 60) / encoderPulsesPerRevolution;
        encoderPulses = 0;
        lastRpmCalcTime = currentMillis;

        int rpmError = desiredRPM - actualMotorRPM;
        int pwmValue = constrain(rpmError * 2, 0, 255);
        analogWrite(motorControlPin, pwmValue);

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

void displayPhSubMenu() {
    lcd.clear();
    for (int i = 0; i < phSubMenuSize; i++) {
        lcd.setCursor(0, i);
        lcd.print((i == phSubMenuIndex) ? "> " : "  ");
        lcd.print(phSubMenuItems[i]);
    }
}

void navigatePhSubMenu(bool clockwise) {
    if (clockwise) phSubMenuIndex = (phSubMenuIndex + 1) % phSubMenuSize;
    else phSubMenuIndex = (phSubMenuIndex - 1 + phSubMenuSize) % phSubMenuSize;
    displayPhSubMenu();
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
    lcd.print(actualMotorRPM);
    lcd.setCursor(0, 2);
    lcd.print("Desired RPM: ");
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

// ======================= DATA DISPLAY =======================
void displayDataPage() {
    lcd.clear();

    // Temperature
    int tempSensorValue = analogRead(tempPin);
    float tempVoltage = tempSensorValue * (5.0 / 1023.0);
    float temperatureC = (55.56 * tempVoltage) + 255.37 - 273.15;

    // pH
    int phSensorValue = analogRead(phPin);
    float phValue = (phSensorValue * 5.0) / 1023.0;

    // Methane
    int methaneSensorValue = analogRead(methaneSensorPin);
    float methaneVoltage = (methaneSensorValue / 1023.0) * 5.0;
    methaneDetected = methaneVoltage > methaneThresholdVoltage;
    String methaneStatus = methaneDetected ? "Detected" : "Not Detected";

    // Display values on LCD
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperatureC, 1);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("pH: ");
    lcd.print(phValue, 2);

    lcd.setCursor(0, 2);
    lcd.print("Methane: ");
    lcd.print(methaneStatus);

    lcd.setCursor(0, 3);
    lcd.print("Motor RPM: ");
    lcd.print(actualMotorRPM);
}


// ======================= PH ASSEMBLY =======================
void runPhAssembly() {
    lcd.clear();
    lcd.print("PH is running...");
    delay(1000);

    unsigned long startTime = millis();
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

    startTime = millis();
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

    lcd.clear();
    lcd.print("PH assembly done");
    delay(2000);
    displayMenu();
}

void setRunTime() {
    lcd.clear();
    lcd.print("Set Runs/Hour: ");
    lcd.setCursor(0, 1);
    lcd.print(phRunTimesPerHour);

    int clkState = digitalRead(clkPin);
    while (true) {
        int newClkState = digitalRead(clkPin);
        if (newClkState != clkState) {
            delay(5);
            int dtState = digitalRead(dtPin);
            phRunTimesPerHour += (dtState != newClkState) ? 1 : -1;
            phRunTimesPerHour = constrain(phRunTimesPerHour, runTimesConstraintMin, runTimesConstraintMax);

            lcd.setCursor(0, 1);
            lcd.print("Runs/Hour: ");
            lcd.print(phRunTimesPerHour);
        }

        if (digitalRead(swPin) == LOW) {
            delay(50);
            if (digitalRead(swPin) == LOW) {
                while (digitalRead(swPin) == LOW);
                break;
            }
        }
    }

    updatePHRunInterval();
    displayMenu();
}

void updatePHRunInterval() {
    phRunInterval = 3600000 / phRunTimesPerHour;
}
