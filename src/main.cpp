#include <Arduino.h>
#include <Unistep2.h>
#include <math.h>
#include <Bounce2.h>
// https://github.com/reven/Unistep2

// TODO: buzzer on timer https://randomnerdtutorials.com/interrupts-timers-esp8266-arduino-ide-nodemcu/
// TODO: prox sensor
// TODO: MQTT

#define STEPS 4096
#define BUZZ D5
#define BTN_FULL D6
#define PIR D7

Bounce debouncer = Bounce(); 
Bounce debouncerPir = Bounce(); 
Unistep2 stepper(D1, D2, D3, D4, 4096, 1000);

bool buzzing = false;

void ICACHE_RAM_ATTR btn_full() {
	Serial.println("full");
	//digitalWrite(BUZZ, HIGH);
	buzzing = true;
	stepper.move(STEPS);
}

// void moveSmall() {
// 	Serial.println("small");
// 	//stepper.move(floor(STEPS / 720.0));
// 	digitalWrite(BUZZ, LOW);
// }

// void ICACHE_RAM_ATTR btn_small() {
// 	Serial.println("trigger");
// 	if (debouncer.fell()) {
// 		moveSmall();
// 	}
// }

// 1) button press
// 2) turn stepper
// 3) when done, start buzzer
// 4) on PIR, stop buzzer
// block button until buzzer is off
// state = waiting, dispensing, buzzing

void ICACHE_RAM_ATTR btn_pir() {
	if (buzzing) {
		Serial.println("cancel");
		buzzing = false;
		digitalWrite(BUZZ, LOW);
	}
}

void setup() {
	Serial.begin(9600);
	debouncer.attach(BTN_FULL, INPUT_PULLUP);
	debouncer.interval(250);

	pinMode(BUZZ, OUTPUT);
	digitalWrite(BUZZ, LOW);
	// pinMode(BTN_FULL, INPUT_PULLUP);
	attachInterrupt(BTN_FULL, btn_full, RISING);
	attachInterrupt(PIR, btn_pir, RISING);

	// debouncerPir.attach(PIR, INPUT);
	// debouncerPir.interval(25);
	Serial.println("Starting 2");
}

void loop() {
	debouncer.update();
	// debouncerPir.update();
	stepper.run();
	 	// Serial.print("v=");
	//  Serial.println(digitalRead(PIR));
	 
	//  delay(500);
}