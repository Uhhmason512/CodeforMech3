#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set up LCD (20x4)
LiquidCrystal_I2C lcd(0x27, 20, 4); 

// Pin Definitions
const int clkPin = 2;          // CLK pin on rotary encoder
const int dtPin = 3;           // DT pin on rotary encoder
const int swPin = 4;           // SW (button) pin on rotary encoder
const int motorControlPin = 7; // DC motor control pin (PWM) for setting RPM
const int tempPin = A0;        // Temperature probe analog input for EI-1034
const int phPin = A1;          // pH probe analog input
const int motorRpmPin = A6;    // Analog pin A6 to read RPM feedback

// Variables for Menu Navigation
int lastClkState;
int menuIndex = 0;
String menuItems[] = {"Heater", "Motor RPM", "Data", "PH Assembly"};
int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);
bool inDataPage = false;       // Flag to indicate if we're in the Data page

// Variables for Motor Control
bool adjustingMotorRPM = false; 
int motorRPM = 0;               // Current motor RPM
int desiredRPM = 0;             // Desired motor RPM set by the user

void setup() {
    pinMode(clkPin, INPUT);
    pinMode(dtPin, INPUT);
    pinMode(swPin, INPUT_PULLUP); 
    pinMode(motorControlPin, OUTPUT);    
    lcd.begin(20, 4);
    lcd.backlight();
    displayMenu();

    lastClkState = digitalRead(clkPin); 
    Serial.begin(9600); // For debugging purposes
}

void loop() {
    int clkState = digitalRead(clkPin); 

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
    
    // Read temperature from EI-1034 temperature probe
    int sensorValue = analogRead(tempPin); // Read analog input (0-1023)
    float voltage = sensorValue * (5.0 / 1023.0); // Convert to voltage (0-5V)
    float temperatureF = voltage / 0.01; // Convert to °F, assuming 10 mV per °F

    // Read pH value (assuming pH probe gives an analog voltage)
    int phValue = analogRead(phPin);
    float phVoltage = phValue * (5.0 / 1023.0); // Convert to voltage
    float phLevel = map(phVoltage, 0, 5, 0, 14); // Placeholder conversion for pH

    // Read motor RPM from analog pin A6
    int rpmValue = analogRead(motorRpmPin); 
    motorRPM = map(rpmValue, 0, 1023, 0, 200); // Map to 0-200 RPM

    // Display Temperature, Motor RPM, pH Level, and "Back" option on LCD
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperatureF);
    lcd.print(" F");

    lcd.setCursor(0, 1);
    lcd.print("Motor RPM: ");
    lcd.print(motorRPM);

    lcd.setCursor(0, 2);
    lcd.print("pH Level: ");
    lcd.print(phLevel);

    lcd.setCursor(0, 3);
    lcd.print("> Back");

    // Print to Serial Monitor for debugging
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.print(" V, Temperature: ");
    Serial.print(temperatureF);
    Serial.println(" °F");
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
