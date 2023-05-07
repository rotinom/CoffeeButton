#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <WiFiManager.h>
#include "secrets.h"

#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "TickTwo.h"

#include "time.h"
#include "LED.h"
#include "SmartButton.h"


#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 0
#define DAYLIGHT_OFFSET_SEC 3600

// The MQTT topics that this device should publish/subscribe
#define COFFEE_TOPIC "i_can_has_coffee/pub"

#define LED_BUILTIN 2
// #define NEO_LED    25
#define RED_BUTTON 25
#define GRN_BUTTON 19

#define RED_LED 26
#define GRN_LED 18

#define LED_HEARTBEAT_MILLIS 30000

char strBuf[256];

// Manage our wifi with a captive portal
WiFiManager wifiManager;
MQTTClient mqttClient(256);
WiFiClientSecure net = WiFiClientSecure();

LED onboardLed(LED_BUILTIN, LED::StateEnum::ON);
LED grnLed(GRN_LED, LED::StateEnum::ON);
LED redLed(RED_LED, LED::StateEnum::ON);

SmartButton* smartGrnButton;
SmartButton* smartRedButton;

#define TICKTWO_FOREVER 0
TickTwo* aliveTimerOn;
TickTwo* aliveTimerOff;


void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}



//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}



void connectWifi(){
    onboardLed.off();

    // Setup the captive portal for wifi setup
    Serial.println("Setting up wifiManager callbacks");
    
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setConfigPortalTimeout(180);
    wifiManager.autoConnect("CoffeeButtonWifi", PORTAL_PASSWORD);
    
    Serial.print("Connecting to Wi-Fi.");
    while (WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
    }
    Serial.println("  Connected!");

    onboardLed.on();
}

void setupMQTT(){
    // Configure WiFiClientSecure to use the AWS IoT device credentials
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);

    // Connect to the MQTT broker on the AWS endpoint we defined earlier
    snprintf(strBuf, sizeof(strBuf), "Beginning connection to MQTT broker at %s", AWS_IOT_ENDPOINT);
    Serial.println(strBuf);
    mqttClient.begin(AWS_IOT_ENDPOINT, 8883, net);

    mqttClient.setTimeout(500);

    snprintf(strBuf, sizeof(strBuf), "Connecting to AWS as %s.", THINGNAME);
    Serial.print(strBuf);
    while (!mqttClient.connect(THINGNAME)) {
        Serial.print(".");
    }
    Serial.println("  Connected!");
}


char jsonBuffer[512];
void publishMessage(const char* topic, const bool& status) {
    StaticJsonDocument<200> doc;
    doc["time"] = getTime();
    doc["status"] = status;
    
    serializeJson(doc, jsonBuffer); // serialize to the buffer

    int count = 0;
    snprintf(strBuf, sizeof(strBuf), "Attempting to publish to topic %s -> %s", topic, jsonBuffer);
    Serial.println(strBuf);

    while(!mqttClient.publish(topic, jsonBuffer)) {
        if(count > 5){
            Serial.println("Timed out trying to publish to MQTT");
            break;
        }

        snprintf(strBuf, sizeof(strBuf), "Last error: %d", mqttClient.lastError());
        Serial.println(strBuf);
        count++;
        delay(1000);
        Serial.println("Trying to send again...");
    }
    if(count <= 5) { 
        snprintf(strBuf, sizeof(strBuf), "Successfully published %s", jsonBuffer);
        Serial.println(strBuf);
    }
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}


void setup() {
    Serial.begin(115200);
    onboardLed.on();

    smartGrnButton = new SmartButton(GRN_BUTTON, INPUT_PULLUP, 50);
    smartGrnButton->setInverted(true);
    smartGrnButton->setPressedCallback(grnButtonClicked);
    smartGrnButton->setReleasedCallback(grnButtonReleased);
    smartGrnButton->setLongPressCallback(grnButtonLongPress);

    smartRedButton = new SmartButton(RED_BUTTON, INPUT_PULLUP, 50);
    smartRedButton->setInverted(true);
    smartRedButton->setPressedCallback(redButtonClicked);
    smartRedButton->setReleasedCallback(redButtonReleased);
    smartRedButton->setLongPressCallback(redButtonLongPress);

    // Declare and start our alive timer
    aliveTimerOn  = new TickTwo(aliveHandlerOn, LED_HEARTBEAT_MILLIS, TICKTWO_FOREVER, MILLIS);
    aliveTimerOn->start();

    // Don't start the off timer
    aliveTimerOff = new TickTwo(aliveHandlerOff,  250, 1, MILLIS);

    // Setup wifi
    connectWifi();

    // Init and get the time
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    Serial.print("Got epoch time of: ");
    Serial.println(getTime());

    // Setup MQTT
    setupMQTT();    
}

bool startedUp = false;

void grnButtonLongPress() {
    Serial.printf("%u - %s:%d Green long-press\n", millis(), __FILE__, __LINE__);
}

void grnButtonClicked(){
    if(!startedUp) return;
    grnLed.setState(LED::StateEnum::ON);
    publishMessage(COFFEE_TOPIC, true);
}

void grnButtonReleased() {
    if(!startedUp) return;
    grnLed.setState(LED::StateEnum::OFF);
}

void redButtonLongPress() {
    Serial.printf("%u - %s:%d Red long-press\n", millis(), __FILE__, __LINE__);
}

void redButtonClicked(){
    if(!startedUp) return;
    redLed.setState(LED::StateEnum::ON);
    publishMessage(COFFEE_TOPIC, false);
}

void redButtonReleased() {
    if(!startedUp) return;
    redLed.setState(LED::StateEnum::OFF);
}

void aliveHandlerOn() {
    grnLed.write(100);
    redLed.write(100);
    aliveTimerOff->start();    
}

void aliveHandlerOff() {
    grnLed.off();
    redLed.off();
}

void tickTimers(){
    aliveTimerOn->update();
    aliveTimerOff->update();
}


int mainLoopFirstInvokedMillis = 0;
void loop() {
    uint32_t loopStartTime = millis();

    tickTimers();

    // Track when the main loop was first invoked
    if(!mainLoopFirstInvokedMillis) {
        mainLoopFirstInvokedMillis = loopStartTime;
    }

    // If we are X ms beyond first start (to let things settle)
    else if(loopStartTime > (mainLoopFirstInvokedMillis + 100)) {
        startedUp = true;
    }

    // To keep the MQTT connection alive
    if(!mqttClient.loop()){
        Serial.printf("MQTT loop failed with error %d\n", mqttClient.lastError());

        if(!mqttClient.connected()){
            Serial.println("Attempting to reconnect...");
            setupMQTT();
        }
    }

    // Update the button
    smartGrnButton->tick();
    smartRedButton->tick();
}
