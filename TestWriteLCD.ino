#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the I2C address for your LCD (0x27 is common, but it may vary)
LiquidCrystal_I2C lcd(0x27, 20, 4); // 16x2 LCD

void setup() {
    lcd.begin(20, 4);  // Initialize the LCD for 20x4 or 16x2
    delay(100);        // Add a 100 ms delay for initialization
    lcd.backlight();
    lcd.setCursor(1, 1);
    lcd.print("Testing...");
}

void loop() {
    // Nothing else to do here
}
