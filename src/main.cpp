#include <Arduino.h>
#include <Stepper.h>
#include <math.h>

#define STEPS 2076 //2038
#define IN1   D1   // IN1 is connected to NodeMCU pin D1 (GPIO5)
#define IN2   D2   // IN2 is connected to NodeMCU pin D2 (GPIO4)
#define IN3   D3   // IN3 is connected to NodeMCU pin D3 (GPIO0)
#define IN4   D4   // IN4 is connected to NodeMCU pin D4 (GPIO2)

Stepper stepper(STEPS, IN4, IN2, IN3, IN1);

void ICACHE_RAM_ATTR btn_full() {
 	Serial.println("One Turn");
	stepper.step(STEPS);
	
}

void ICACHE_RAM_ATTR btn_small() {
 	Serial.println("Small Turn");
	stepper.step(1);
}

void setup() {
	Serial.begin(115200);

  	stepper.setSpeed(5);
	pinMode(D6, INPUT_PULLUP);
	pinMode(D7, INPUT_PULLUP);
	attachInterrupt(D6, btn_full, RISING);
	attachInterrupt(D7, btn_small, RISING);
}

void loop() {
	// stepper.step(1624);
	// digitalWrite(IN1, LOW);
	// digitalWrite(IN2, LOW);
	// digitalWrite(IN3, LOW);
	// digitalWrite(IN4, LOW);
	// delay(5000);
	yield();
}