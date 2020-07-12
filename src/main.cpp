#include <Arduino.h>
#include <Unistep2.h>
#include <math.h>
#include <Bounce2.h>
// https://github.com/reven/Unistep2

// TODO: debounce https://github.com/thomasfredericks/Bounce2
// TODO: buzzer on timer https://randomnerdtutorials.com/interrupts-timers-esp8266-arduino-ide-nodemcu/

#define STEPS 4096

Bounce debouncer = Bounce(); 
Unistep2 stepper(D1, D2, D3, D4, 4096, 1000);

void ICACHE_RAM_ATTR btn_full() {
	Serial.println("full");
	stepper.move(STEPS);
}

void moveSmall() {
	Serial.println("small");
	stepper.move(floor(STEPS / 720.0));
}

void ICACHE_RAM_ATTR btn_small() {
	Serial.println("trigger");
	if (debouncer.fell()) {
		moveSmall();
	}
}

void setup() {
	Serial.begin(115200);

	pinMode(D6, INPUT_PULLUP);
	//pinMode(D7, INPUT_PULLUP);
	attachInterrupt(D6, btn_full, RISING);
	attachInterrupt(D7, btn_small, RISING);

	debouncer.attach(D7, INPUT_PULLUP);
	debouncer.interval(25);
}

void loop() {
	debouncer.update();
	 stepper.run();
}