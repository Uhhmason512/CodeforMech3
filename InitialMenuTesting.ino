#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // Set up LCD (20x4)

const int clkPin = 2;  // CLK pin on rotary encoder
const int dtPin = 3;   // DT pin on rotary encoder
const int swPin = 4;   // SW (button) pin on rotary encoder

int lastClkState;
int menuIndex = 0;
String menuItems[] = {"Heater", "Motor RPM", "PH Assembly", "Calibration"};
int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);

void setup() {
    pinMode(clkPin, INPUT);
    pinMode(dtPin, INPUT);
    pinMode(swPin, INPUT_PULLUP); // Use internal pull-up for the switch
    lcd.begin(20, 4);
    lcd.backlight();
    displayMenu();

    lastClkState = digitalRead(clkPin); // Initial state of CLK
}

void loop() {
    int clkState = digitalRead(clkPin); // Read the CLK state

    // Check for rotation
    if (clkState != lastClkState) { // State changed
        if (digitalRead(dtPin) != clkState) { // Determine direction
            menuIndex = (menuIndex + 1) % menuSize; // Rotate right
        } else {
            menuIndex = (menuIndex - 1 + menuSize) % menuSize; // Rotate left
        }
        displayMenu(); // Update the display
    }
    lastClkState = clkState; // Update last CLK state

    // Check for button press
    if (digitalRead(swPin) == LOW) {
        delay(50); // Debounce delay
        if (digitalRead(swPin) == LOW) { // Confirm button press
            selectMenuItem();
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

void selectMenuItem() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Selected: ");
    lcd.setCursor(0, 1);
    lcd.print(menuItems[menuIndex]);
    delay(1000); // Display selected item briefly
    displayMenu(); // Return to menu
}
