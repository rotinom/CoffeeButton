#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <WiFiManager.h>
#include "secrets.h"

#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <AcksenButton.h>
// #include <Adafruit_NeoPixel.h>
// #include "TickTwo.h"

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

#define strBufSize 256
char strBuf[strBufSize];

// Manage our wifi with a captive portal
WiFiManager wifiManager;
MQTTClient client(256);
WiFiClientSecure net = WiFiClientSecure();

// AcksenButton grnButton(GRN_BUTTON, ACKSEN_BUTTON_MODE_NORMAL|ACKSEN_BUTTON_MODE_LONGPRESS, BUTTON_DEBOUNCE_INTERVAL, INPUT_PULLUP);
AcksenButton redButton(RED_BUTTON, ACKSEN_BUTTON_MODE_NORMAL|ACKSEN_BUTTON_MODE_LONGPRESS, 100, INPUT_PULLUP);

LED onboardLed(LED_BUILTIN);
LED grnLed(GRN_LED);
LED redLed(RED_LED);

SmartButton* smartGrnButton;




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

// void setWlanLED(bool state){
//   digitalWrite(LED_BUILTIN, state);
// }

// void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
//   Serial.println("Connected to AP successfully!");
//   setWlanLED(true);
// }

// void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
//   Serial.println("Connected to AP successfully!");
//   setWlanLED(false);
// }

void connectWifi(){
    Serial.print("Connecting to Wi-Fi.");
    while (WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
    }
    Serial.println("  Connected!");
}

void setupMQTT(){
    // Configure WiFiClientSecure to use the AWS IoT device credentials
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);

    // Connect to the MQTT broker on the AWS endpoint we defined earlier
    snprintf(strBuf, strBufSize, "Beginning connection to MQTT broker at %s", AWS_IOT_ENDPOINT);
    Serial.println(strBuf);
    client.begin(AWS_IOT_ENDPOINT, 8883, net);

    snprintf(strBuf, strBufSize, "Connecting to AWS as %s.", THINGNAME);
    Serial.print(strBuf);
    while (!client.connect(THINGNAME)) {
      Serial.print(".");
    }

    // TODO: Look at timeouts
    // if(!client.connected()){
    //   Serial.println("Timed out!");
    //   return;
    // }
    // else {
      Serial.println("  Connected!");
    // }
}

char jsonBuffer[512];
void publishMessage(const char* topic, const bool& status) {
    StaticJsonDocument<200> doc;
    doc["time"] = getTime();
    doc["status"] = status;
    
    serializeJson(doc, jsonBuffer); // serialize to the buffer

    int count = 0;
    snprintf(strBuf, strBufSize, "Attempting to publish to topic %s -> %s", topic, jsonBuffer);
    Serial.println(strBuf);

    while(!client.publish(topic, jsonBuffer)) {
        if(count > 5){
            Serial.println("Timed out trying to publish to MQTT");
            break;
        }

        snprintf(strBuf, strBufSize, "Last error: %d", client.lastError());
        Serial.println(strBuf);
        count++;
        delay(1000);
        Serial.println("Trying to send again...");
    }
    if(count <= 5) { 
        snprintf(strBuf, strBufSize, "Successfully published %s", jsonBuffer);
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
    Serial.println("Setting up initial LED state");
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); 

    smartGrnButton = new SmartButton(GRN_BUTTON, INPUT_PULLUP, 50);
    smartGrnButton->setInverted(true);
    smartGrnButton->setPressedCallback(grnButtonClicked);
    smartGrnButton->setReleasedCallback(grnButtonReleased);

    // grnButton.setLongPressInterval(1000);
    redButton.setLongPressInterval(1000);


    // pinMode(RED_BUTTON, INPUT_PULLUP);
    // pinMode(GRN_BUTTON, INPUT_PULLUP);

    // Hook up callbacks for wifi status
    // Serial.println("Hooking up Wifi callbacks");
    // Serial.flush();
    // WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    // WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    // // Start our tickers
    // Serial.println("Starting tickers");
    // ledTicker.start(); //start the ticker.

    // Setup the captive portal for wifi setup
    Serial.println("Setting up wifiManager callbacks");
    
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setConfigPortalTimeout(180);
    wifiManager.autoConnect("CoffeeButtonWifi", PORTAL_PASSWORD);

    // Setup wifi
    connectWifi();

    // Init and get the time
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    Serial.print("Got epoch time of: ");
    Serial.println(getTime());

    // Setup MQTT
    setupMQTT();

    
}

void setGrnLight(uint8_t state){
    grnLed.setState(state);
}

void setRedLight(uint8_t state){
    redLed.setState(state);
}

void grnButtonClicked(){
    grnLed.setState(state);
    publishMessage(COFFEE_TOPIC, true);
}

void grnButtonReleased() {
    setGrnLight(LOW);
}

void redButtonClicked(){
    grnLed.setState(state);
    publishMessage(COFFEE_TOPIC, true);
}

void redButtonReleased() {
    setGrnLight(LOW);
}

bool grnLongPress = false;
bool redLongPress = false;

void loop() {
    // To keep the MQTT connection alive
    client.loop();

    // Update the button
    smartGrnButton->tick();

    // grnButton.refreshStatus();
    // if(grnButton.onReleased()) {
    //     snprintf(strBuf, strBufSize, "%d I CAN HAS COFFEE! 😸", millis());
    //     Serial.println(strBuf);
    //     Serial.println("pressed...");

    //     setGrnLight(HIGH);
    //     publishMessage(COFFEE_TOPIC, true);
    // }
    // else if(grnButton.onPressed()){
    //     setGrnLight(LOW);
    //     Serial.println("released...");
    // }

    redButton.refreshStatus();
    if(redButton.onReleased()) {
        snprintf(strBuf, strBufSize, "%d I NO CAN HAS COFFEE! :(", millis());
        Serial.println(strBuf);
        Serial.println("pressed...");

        setRedLight(HIGH);
        publishMessage(COFFEE_TOPIC, false);
    }
    else if(redButton.onPressed()){
        setRedLight(LOW);
        Serial.println("released...");
    }

    // if(grnButton.onLongPress()){
    //   Serial.println("Green long-pressed");
    //   grnLongPress = true;
    // }
    // if(grnButton.onReleased()){
    //   grnLongPress = false;
    // }


    if(redButton.onLongPress()){
      Serial.println("Red long-pressed");
      redLongPress = true;
    }
    if(redButton.onReleased()){
      redLongPress = false;
    }

}
