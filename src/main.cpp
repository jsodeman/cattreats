#include <Arduino.h>
#include <Unistep2.h>
#include <math.h>
#include <Bounce2.h>
// https://github.com/reven/Unistep2

// TODO: MQTT
// TODO: change button to small adjustment, dispense only from app

#define STEPS 4096
#define BUZZ D5
#define BTN_FULL D6
#define PIR D7
const long BUZZ_ON = 500;
const long BUZZ_OFF = 5000;
const int STATE_WAIT = 1;
const int STATE_DISPENSE = 2;
const int STATE_BUZZ = 3;
// const int STATE_RESET = 4;

int state = STATE_WAIT; 
bool buzzing = false;
long buzzTimer = 0;

// Bounce debouncer = Bounce(); 
// Bounce debouncerPir = Bounce(); 
Unistep2 stepper(D1, D2, D3, D4, 4096, 1000);

void ICACHE_RAM_ATTR btn_full() {
	if (state != STATE_WAIT) {
		return;
	} 

	state = STATE_DISPENSE;
	Serial.println("dispensing");
	detachInterrupt(BTN_FULL);
	//digitalWrite(BUZZ, HIGH);
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
	if (state != STATE_BUZZ) {
		return;
	}
	digitalWrite(BUZZ, LOW);
	buzzing = false;
	buzzTimer = 0;
	Serial.println("buzz stop");
	state = STATE_WAIT;
	detachInterrupt(PIR);
	attachInterrupt(BTN_FULL, btn_full, RISING);
}

void setup() {
	Serial.begin(9600);
	// debouncer.attach(BTN_FULL, INPUT_PULLUP);
	// debouncer.interval(250);

	pinMode(BTN_FULL, INPUT_PULLUP);
	pinMode(BUZZ, OUTPUT);
	pinMode(PIR, INPUT);
	digitalWrite(BUZZ, LOW);
	attachInterrupt(BTN_FULL, btn_full, RISING);
	
	// debouncerPir.attach(PIR, INPUT);
	// debouncerPir.interval(25);
	Serial.println("Starting");
}

void loop() {
	// debouncerPir.update();
	stepper.run();

	if (stepper.stepsToGo() == 0 && state == STATE_DISPENSE) {
		state = STATE_BUZZ;
		digitalWrite(BUZZ, HIGH);
		buzzing = true;
		buzzTimer = millis();
		attachInterrupt(PIR, btn_pir, RISING);
		Serial.println("dispense done");
		Serial.println("buzzing");
	}

	if (state == STATE_BUZZ) {
		long now = millis();

		if (buzzing && (now - buzzTimer >= BUZZ_ON)) {
			digitalWrite(BUZZ, LOW);
			buzzing = false;
			buzzTimer = now;
		} else if (!buzzing && (now - buzzTimer >= BUZZ_OFF)) {
			digitalWrite(BUZZ, HIGH);
			buzzing = true;
			buzzTimer = now;
		}
	}
}