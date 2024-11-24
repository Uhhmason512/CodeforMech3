#define PWM_PIN 9  // PWM control pin (blue wire)
#define DELAY_TIME 50  // Delay between speed changes (in milliseconds)

void setup() {
  pinMode(PWM_PIN, OUTPUT); // Set the PWM pin as output
}

void loop() {
  // Gradually increase the speed
  for (int speed = 0; speed <= 255; speed += 5) {
    analogWrite(PWM_PIN, speed); // Set motor speed
    delay(DELAY_TIME);           // Wait before increasing speed
  }

  // Gradually decrease the speed
  for (int speed = 255; speed >= 0; speed -= 5) {
    analogWrite(PWM_PIN, speed); // Set motor speed
    delay(DELAY_TIME);           // Wait before decreasing speed
  }
}
