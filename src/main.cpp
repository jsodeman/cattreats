#include <Arduino.h>
#include <Unistep2.h>
#include <math.h>
#include <Bounce2.h>
#include <IotWebConf.h>
#include <MQTT.h>

// TODO: replace mosquittos with https://github.com/chkr1011/MQTTnet
// TODO: build website for controlling

// ----------- Wifi and MQTT
const char thingName[] = "catTreats1";
const char wifiInitialApPassword[] = "mewmewmew";

#define STRING_LEN 128
#define CONFIG_VERSION "mqt1"

void wifiConnected();
void configSaved();
void handleRoot();
boolean connectMqtt();
boolean formValidator();
boolean connectMqttOptions();
void mqttMessageReceived(String &topic, String &payload);

DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;
WiFiClient net;
MQTTClient mqttClient;

char mqttServerValue[STRING_LEN];
char mqttUserNameValue[STRING_LEN];
char mqttUserPasswordValue[STRING_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
IotWebConfParameter mqttServerParam = IotWebConfParameter("MQTT server", "mqttServer", mqttServerValue, STRING_LEN);
IotWebConfParameter mqttUserNameParam = IotWebConfParameter("MQTT user", "mqttUser", mqttUserNameValue, STRING_LEN);
IotWebConfParameter mqttUserPasswordParam = IotWebConfParameter("MQTT password", "mqttPass", mqttUserPasswordValue, STRING_LEN, "password");

boolean needMqttConnect = false;
boolean needReset = false;
unsigned long lastReport = 0;
unsigned long lastMqttConnectionAttempt = 0;

// ----------- App
// https://github.com/reven/Unistep2

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
const int RESET_DELAY = 1000;

int state = STATE_WAIT; 
bool buzzing = false;
long buzzTimer = 0;
long resetClock = 0;

// Bounce debouncer = Bounce(); 
Unistep2 stepper(D1, D2, D3, D4, 4096, 1000);

void ICACHE_RAM_ATTR btn_full() {
	if (state != STATE_WAIT) {
		return;
	}

	if (millis() - resetClock < RESET_DELAY) {
		Serial.println("false trigger");
		return;
	}

	detachInterrupt(BTN_FULL);
	state = STATE_DISPENSE;
	Serial.println("dispensing");
	mqttClient.publish("/test/status", "treating");
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
	resetClock = millis();
	attachInterrupt(BTN_FULL, btn_full, RISING);
}

void setup() {
	Serial.begin(115200);

	// Wifi / MQTT --------------------------------------
	iotWebConf.addParameter(&mqttServerParam);
	iotWebConf.addParameter(&mqttUserNameParam);
	iotWebConf.addParameter(&mqttUserPasswordParam);
	iotWebConf.setConfigSavedCallback(&configSaved);
	iotWebConf.setFormValidator(&formValidator);
	iotWebConf.setWifiConnectionCallback(&wifiConnected);
	iotWebConf.setupUpdateServer(&httpUpdater);

	// -- Initializing the configuration.
	boolean validConfig = iotWebConf.init();
	if (!validConfig)
	{
		mqttServerValue[0] = '\0';
		mqttUserNameValue[0] = '\0';
		mqttUserPasswordValue[0] = '\0';
	}

	// -- Set up required URL handlers on the web server.
	server.on("/", handleRoot);
	server.on("/config", []{ iotWebConf.handleConfig(); });
	server.onNotFound([](){ iotWebConf.handleNotFound(); });

	mqttClient.begin(mqttServerValue, net);
	mqttClient.onMessage(mqttMessageReceived);


	// App ----------------------------------------------

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
	// Wifi / MQTT --------------------------------------
	iotWebConf.doLoop();
	mqttClient.loop();

	if (needMqttConnect)
	{
		if (connectMqtt())
	{
		needMqttConnect = false;
	}
	}
		else if ((iotWebConf.getState() == IOTWEBCONF_STATE_ONLINE) && (!mqttClient.connected()))
	{
		Serial.println("MQTT reconnect");
		connectMqtt();
	}

	if (needReset)
	{
		Serial.println("Rebooting after 1 second.");
		iotWebConf.delay(1000);
		ESP.restart();
	}

	unsigned long now = millis();
	// if ((500 < now - lastReport) && (pinState != digitalRead(CONFIG_PIN)))
	// {
	// 	pinState = 1 - pinState; // invert pin state as it is changed
	// 	lastReport = now;
	// 	Serial.print("Sending on MQTT channel '/test/status' :");
	// 	Serial.println(pinState == LOW ? "ON" : "OFF");
	// 	mqttClient.publish("/test/status", pinState == LOW ? "ON" : "OFF");
	// }

	// App ----------------------------------------------
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

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 06 MQTT App</title></head><body>MQTT App demo";
  s += "<ul>";
  s += "<li>MQTT server: ";
  s += mqttServerValue;
  s += "</ul>";
  s += "Go to <a href='config'>configure page</a> to change values.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void wifiConnected()
{
  needMqttConnect = true;
}

void configSaved()
{
  Serial.println("Configuration was updated.");
  needReset = true;
}

boolean formValidator()
{
  Serial.println("Validating form.");
  boolean valid = true;

  int l = server.arg(mqttServerParam.getId()).length();
  if (l < 3)
  {
    mqttServerParam.errorMessage = "Please provide at least 3 characters!";
    valid = false;
  }

  return valid;
}

boolean connectMqtt() {
  unsigned long now = millis();
  if (1000 > now - lastMqttConnectionAttempt)
  {
    // Do not repeat within 1 sec.
    return false;
  }
  Serial.println("Connecting to MQTT server...");
  if (!connectMqttOptions()) {
    lastMqttConnectionAttempt = now;
    return false;
  }
  Serial.println("Connected!");

  mqttClient.subscribe("/test/action");
  return true;
}

/*
// -- This is an alternative MQTT connection method.
boolean connectMqtt() {
  Serial.println("Connecting to MQTT server...");
  while (!connectMqttOptions()) {
    iotWebConf.delay(1000);
  }
  Serial.println("Connected!");
  mqttClient.subscribe("/test/action");
  return true;
}
*/

boolean connectMqttOptions()
{
  boolean result;
  if (mqttUserPasswordValue[0] != '\0')
  {
    result = mqttClient.connect(iotWebConf.getThingName(), mqttUserNameValue, mqttUserPasswordValue);
  }
  else if (mqttUserNameValue[0] != '\0')
  {
    result = mqttClient.connect(iotWebConf.getThingName(), mqttUserNameValue);
  }
  else
  {
    result = mqttClient.connect(iotWebConf.getThingName());
  }
  return result;
}

void mqttMessageReceived(String &topic, String &payload)
{
  Serial.println("Incoming: " + topic + " - " + payload);
  btn_full();
}