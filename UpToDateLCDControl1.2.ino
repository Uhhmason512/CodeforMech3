#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <util/atomic.h> // For the ATOMIC_BLOCK macro
#include <Arduino.h>
#include <Servo.h>
#include <ezButton.h>

// LCD Setup (20x4)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Pin Definitions for Menu and Sensors
const int clkPin = 2;          // Rotary encoder CLK
const int dtPin = 3;           // Rotary encoder DT
const int swPin = 4;           // Rotary encoder button
const int tempPin = A0;        // EI-1034 temperature probe
const int phPin = A1;          // pH probe

// Pin Definitions for Motor Control
const int controlPin1 = 42;    // H-Bridge pin 1
const int controlPin2 = 4;     // H-Bridge pin 2
const int enablePin = 9;       // PWM for motor speed
const int relayPin = 8;        // Relay for heater
#define ENCA 18                // Encoder output A
#define ENCB 19                // Encoder output B

// Variables for Menu Navigation
int lastClkState;
int menuIndex = 0;
String menuItems[] = {"Heater", "Motor RPM", "Data", "PH Assembly"};
int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);
bool inDataPage = false;

// Variables for Heater Control
float desiredTemp = 40.0;      // Desired temperature (Celsius)
bool heaterOn = false;

// Variables for Motor Control
bool adjustingMotorRPM = false;
int motorRPM = 0;              // Current motor RPM
int desiredRPM = 0;            // Desired RPM set by user
volatile int posi = 0;         // Encoder position (volatile for interrupt safety)

// Variables for Timing and RPM Calculation
unsigned long prev_time = 0;
unsigned long lastRefreshTime = 0;
const unsigned long refreshInterval = 1000; // 1 second refresh
float RPM;

// Variables for pH Assembly
int phRunsPerHour = 6;         // Default: 6 runs/hour
unsigned long phLastRunTime = 0; // Timestamp of last pH assembly run
Servo myservo;
ezButton limitSwitch(48);

void setup() {
    // Pin Modes
    pinMode(clkPin, INPUT);
    pinMode(dtPin, INPUT);
    pinMode(swPin, INPUT_PULLUP);
    pinMode(controlPin1, OUTPUT);
    pinMode(controlPin2, OUTPUT);
    pinMode(enablePin, OUTPUT);
    pinMode(relayPin, OUTPUT);
    pinMode(ENCA, INPUT);
    pinMode(ENCB, INPUT);
    digitalWrite(relayPin, LOW); // Ensure relay is off initially

    // Interrupt for Encoder
    attachInterrupt(digitalPinToInterrupt(ENCA), readEncoder, RISING);

    // LCD Initialization
    lcd.begin(20, 4);
    lcd.backlight();
    displayMenu();

    // pH Assembly Setup
    myservo.attach(49);
    myservo.write(115);
    limitSwitch.setDebounceTime(50);

    // Serial Monitor for Debugging
    Serial.begin(9600);
    lastClkState = digitalRead(clkPin);
}

void loop() {
    int clkState = digitalRead(clkPin); // Read the rotary encoder CLK state

    // Refresh Data Page
    if (inDataPage) {
        unsigned long currentTime = millis();
        if (currentTime - lastRefreshTime >= refreshInterval) {
            lastRefreshTime = currentTime;
            displayDataPage();
        }
    }

    // Rotary Encoder Navigation
    if (clkState != lastClkState) {
        delay(5); // Stabilize signal
        int dtState = digitalRead(dtPin);

        if (menuItems[menuIndex] == "PH Assembly" && adjustingMotorRPM) {
            adjustPHRunsPerHour(dtState != clkState);
        } else {
            // Navigate Menu
            if (dtState != clkState) {
                menuIndex = (menuIndex + 1) % menuSize;
            } else {
                menuIndex = (menuIndex - 1 + menuSize) % menuSize;
            }
            displayMenu();
        }
    }
    lastClkState = clkState;

    // Button Press Handling
    if (digitalRead(swPin) == LOW) {
        delay(50); // Debounce
        if (digitalRead(swPin) == LOW) {
            if (inDataPage) {
                inDataPage = false;
                displayMenu();
            } else if (menuItems[menuIndex] == "PH Assembly") {
                handlePHMenuSelection();
            } else if (menuItems[menuIndex] == "Motor RPM") {
                handleMotorRPMSelection();
            }
            while (digitalRead(swPin) == LOW); // Wait for release
        }
    }

    // Periodically Run pH Assembly
    runPHAssemblyPeriodically();

    // Motor Control Logic
    controlMotor();

    // Heater Control Logic
    manageHeater();
}

// Display Functions
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

void displayPHAssemblyMenu() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PH Assembly");
    lcd.setCursor(0, 1);
    lcd.print("1. Run");
    lcd.setCursor(0, 2);
    lcd.print("2. Set Per Hour");
}

void handlePHMenuSelection() {
    static int phSubMenuIndex = 0;

    if (phSubMenuIndex == 0) {
        executePHAssembly();
    } else if (phSubMenuIndex == 1) {
        setPHRunsPerHour();
    }
}

void setPHRunsPerHour() {
    adjustingMotorRPM = true;
    while (adjustingMotorRPM) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Runs/Hour: ");
        lcd.print(phRunsPerHour);
        lcd.setCursor(0, 1);
        lcd.print("Press to Save");
        if (digitalRead(swPin) == LOW) {
            delay(50);
            adjustingMotorRPM = false;
        }
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Saved: ");
    lcd.print(phRunsPerHour);
    lcd.print(" /hour");
    delay(2000);
}

void executePHAssembly() {
    // Add the full pH Assembly functionality here
    Serial.println("Executing PH Assembly...");
}

void runPHAssemblyPeriodically() {
    unsigned long currentTime = millis();
    if (currentTime - phLastRunTime >= (60 * 60 * 1000 / phRunsPerHour)) {
        phLastRunTime = currentTime;
        executePHAssembly();
    }
}

void adjustPHRunsPerHour(bool increment) {
    if (increment) {
        phRunsPerHour++;
    } else {
        phRunsPerHour--;
    }
    phRunsPerHour = constrain(phRunsPerHour, 1, 60);
}

// Remaining Functions (unchanged):
// - updateMotorRPMDisplay
// - displayDataPage
// - controlMotor
// - manageHeater
// - readTemperature
// - readEncoder
