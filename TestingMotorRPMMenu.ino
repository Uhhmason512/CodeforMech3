#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // Set up LCD (20x4)

// Pin Definitions
const int clkPin = 2;       // CLK pin on rotary encoder
const int dtPin = 3;        // DT pin on rotary encoder
const int swPin = 4;        // SW (button) pin on rotary encoder
const int motorPin = 7;     // DC motor control pin (PWM)

// Variables for Menu Navigation
int lastClkState;
int menuIndex = 0;
String menuItems[] = {"Heater", "Motor RPM", "Data", "PH Assembly"};
int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);

// Variables for Motor Control
bool adjustingMotorRPM = false; // Flag to track if we're in RPM adjustment mode
int motorRPM = 0;               // Current motor RPM
int desiredRPM = 0;             // Desired motor RPM set by the user

void setup() {
    pinMode(clkPin, INPUT);
    pinMode(dtPin, INPUT);
    pinMode(swPin, INPUT_PULLUP); // Use internal pull-up for the switch
    pinMode(motorPin, OUTPUT);    // Motor control pin
    lcd.begin(20, 4);
    lcd.backlight();
    displayMenu();

    lastClkState = digitalRead(clkPin); // Initial state of CLK
}

void loop() {
    int clkState = digitalRead(clkPin); // Read the CLK state

    // Check for rotation
    if (clkState != lastClkState) { // State changed
        if (adjustingMotorRPM) {
            // Adjust motor RPM based on rotation direction
            if (digitalRead(dtPin) != clkState) { 
                desiredRPM += 10; // Rotate right: increase RPM by 10
            } else {
                desiredRPM -= 10; // Rotate left: decrease RPM by 10
            }
            desiredRPM = constrain(desiredRPM, 0, 150); // Limit RPM within range
            updateMotorRPMDisplay(); // Update RPM display
        } else {
            // Normal menu navigation
            if (digitalRead(dtPin) != clkState) {
                menuIndex = (menuIndex + 1) % menuSize; // Rotate right: next menu item
            } else {
                menuIndex = (menuIndex - 1 + menuSize) % menuSize; // Rotate left: previous menu item
            }
            displayMenu(); // Update the displayed menu
        }
    }
    lastClkState = clkState; // Update last CLK state

    // Check for button press
    if (digitalRead(swPin) == LOW) {
        delay(50); // Debounce delay
        if (digitalRead(swPin) == LOW) { // Confirm button press
            if (menuItems[menuIndex] == "Motor RPM") {
                if (!adjustingMotorRPM) {
                    // Enter motor RPM adjustment mode
                    adjustingMotorRPM = true;
                    desiredRPM = motorRPM; // Set desired RPM to current RPM
                    updateMotorRPMDisplay();
                } else {
                    // Set the new RPM and exit adjustment mode
                    motorRPM = desiredRPM;
                    analogWrite(motorPin, map(motorRPM, 0, 150, 0, 255)); // Apply new RPM to motor
                    adjustingMotorRPM = false;
                    displayMenu(); // Return to menu
                }
            } else {
                selectMenuItem(); // Handle other menu items
            }
            while (digitalRead(swPin) == LOW); // Wait for button release
        }
    }
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

void selectMenuItem() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Selected: ");
    lcd.setCursor(0, 1);
    lcd.print(menuItems[menuIndex]);
    delay(1000); // Display selected item briefly
    displayMenu(); // Return to menu
}

