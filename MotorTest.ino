const int DIR_PIN = 8;    // Direction control wire
const int PWM_PIN = 9;    // Speed control wire (PWM signal)

void setup() {
  pinMode(DIR_PIN, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);
  Serial.begin(9600); // Initialize Serial Monitor
}

void loop() {
  // Rotate Clockwise at 50% speed
  digitalWrite(DIR_PIN, HIGH); // Set direction to CW
  analogWrite(PWM_PIN, 128);   // Set speed (50% duty cycle)
  Serial.println("Clockwise at 50% speed");
  delay(5000); // Run for 5 seconds

  // Stop motor before switching direction
  analogWrite(PWM_PIN, 0);     // Stop motor
  delay(500);                  // Short delay
  Serial.println("Motor stopped");

  // Rotate Counterclockwise at 75% speed
  digitalWrite(DIR_PIN, LOW);  // Set direction to CCW
  analogWrite(PWM_PIN, 192);   // Set speed (75% duty cycle)
  Serial.println("Counterclockwise at 75% speed");
  delay(5000); // Run for 5 seconds
}
