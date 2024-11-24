#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Set up LCD (20x4)
LiquidCrystal_I2C lcd(0x27, 20, 4); 

// Pin Definitions
const int clkPin = 2;          // CLK pin on rotary encoder - Determines Input for Movement
const int dtPin = 3;           // DT pin on rotary encoder - Determines Change of Direction with a 2nd Pulse
const int swPin = 4;           // SW (button) pin on rotary encoder
const int motorControlPin = 7; // DC motor control pin (PWM) for setting RPM
const int tempPin = 6;         // Temperature sensor pin (OneWire for DS18B20)
const int phPin = 5;           // pH probe digital input
const int motorRpmPin = A6;    // Analog pin A6 to read RPM feedback

// Variables for Menu Navigation
int lastClkState;
int menuIndex = 0;
String menuItems[] = {"Heater", "Motor RPM", "Data", "PH Assembly"};
int menuSize = sizeof(menuItems) / sizeof(menuItems[0]); //essential because c measures in bytes
bool inDataPage = false;       // Flag to indicate if we're in the Data page

// Variables for Motor Control
bool adjustingMotorRPM = false; 
int motorRPM = 0;               // Current motor RPM
int desiredRPM = 0;             // Desired motor RPM set by the user

// Set up OneWire and DallasTemperature for thermometer (DS18B20)
OneWire oneWire(tempPin);
DallasTemperature sensors(&oneWire);

void setup() {
    pinMode(clkPin, INPUT);
    pinMode(dtPin, INPUT);
    pinMode(swPin, INPUT_PULLUP); 
    pinMode(motorControlPin, OUTPUT);    
    lcd.begin(20, 4);
    lcd.backlight();
    sensors.begin();  // Initialize the temperature sensor
    displayMenu();

    lastClkState = digitalRead(clkPin); //reads rotation
}

void loop() {
    int clkState = digitalRead(clkPin);  //initialize

    // Check for rotation
    if (clkState != lastClkState) { 
        if (inDataPage) {
            // Rotate within Data page, specifically to select "Back"
            displayDataPage();
        } else if (adjustingMotorRPM) {
            // Adjust motor RPM
            if (digitalRead(dtPin) != clkState) { 
                desiredRPM += 10; 
            } else {
                desiredRPM -= 10; 
            }
            desiredRPM = constrain(desiredRPM, 0, 150); 
            updateMotorRPMDisplay(); 
        } else {
            // Navigate main menu
            if (digitalRead(dtPin) != clkState) {
                menuIndex = (menuIndex + 1) % menuSize; 
            } else {
                menuIndex = (menuIndex - 1 + menuSize) % menuSize; 
            }
            displayMenu(); 
        }
    }
    lastClkState = clkState; 

    // Check for button press
    if (digitalRead(swPin) == LOW) {
        delay(50); 
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
            } else if (menuItems[menuIndex] == "Data") {
                inDataPage = true;
                displayDataPage();
            } else {
                selectMenuItem(); 
            }
            while (digitalRead(swPin) == LOW); 
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

void displayDataPage() {
    lcd.clear();
    
    // Read temperature
    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0);

    // Read pH value (assuming pH probe gives an analog voltage)
    int phValue = analogRead(phPin); // changed to analogRead for example
    float phVoltage = phValue * (5.0 / 1023.0); // Convert to voltage
    float phLevel = map(phVoltage, 0, 5, 0, 14); // Placeholder conversion for pH

    // Read motor RPM from analog pin A6
    int rpmValue = analogRead(motorRpmPin); 
    motorRPM = map(rpmValue, 0, 1023, 0, 200); // Map to 0-200 RPM

    // Display all data and the "Back" option on the LCD
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("Motor RPM: ");
    lcd.print(motorRPM);

    lcd.setCursor(0, 2);
    lcd.print("pH Level: ");
    lcd.print(phLevel);

    lcd.setCursor(0, 3);
    lcd.print("> Back");

    // Wait for user to press button to go back
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
