
#define MQTT_KEEPALIVE 5

#include <ESP8266WiFi.h>
//#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
//#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
//#include <WiFiServer.h>
//#include <WiFiUdp.h>

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>

#define wifi_ssid "xxxx"
#define wifi_password "xxxxxxxxxx"

#define mqtt_server "192.168.0.203"
#define mqtt_user ""
#define mqtt_password ""

//Tämmä täytyy olla kaikille MQTT laitteille uniikki
#define MQTT_CLIENT_ID "IoTProjektiIlto"

#define my_topic_data   "ilto/data"
#define my_topic_ctrl   "ilto"
#define my_topic_state  "ilto/state"
#define my_topic_wifi   "ilto/wifi/main"
#define my_topic_trace  "ilto/trace"

#define MAX_UART_CHARS 200

void msg_callback(const char* topic, byte* payload, unsigned int len);

WiFiClient espClient;
//PubSubClient client(espClient);
PubSubClient client(mqtt_server, 1883, msg_callback, espClient);

#define tracing false
void trace(const char * text)
{
  if(tracing == true)
  {
    Serial.println(text);
  }
}

void setup_wifi() {
  // delay(10);
  // We start by connecting to a WiFi network
  if(WiFi.status() != WL_CONNECTED)
  {
    trace("Connecting to wifi");
  
    WiFi.begin(wifi_ssid, wifi_password);
  
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      trace(".");
      Serial.print("Q:");
    }
    trace("WiFi connected");
    //WiFi.localIP()
    Serial.print("QW");
  }
}

void msg_callback(const char * topic, byte* payload, unsigned int len) {
  //Serial.println(topic);
  for (int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
  
void chkconnect() {
  if (!client.connected()) {
    setup_wifi();
    // Loop until we're reconnected
    while (!client.connected()) {
      trace("Attempting MQTT connection...");
      if (client.connect(MQTT_CLIENT_ID, my_topic_state, MQTTQOS1, true, "offline")) {
        trace("connected");
        Serial.print("QC");
        client.publish(my_topic_state, "online", true);
        client.publish(my_topic_trace, "ILTO connected", false);
        client.subscribe(my_topic_ctrl, MQTTQOS0);
      } else {
        Serial.print("Q.");
        delay(100);
      }
    }
  }
  client.loop();
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  //client.setServer(mqtt_server, 1883);
  //client.setCallback(msg_callback);
  delay(1000);
  chkconnect();
}

void serialEvent() {
  String inputString = "";         // a string to hold incoming data
  inputString.reserve(MAX_UART_CHARS);

  if (Serial.peek() > 0)
  {
    inputString = Serial.readString();
    trace(inputString.c_str());
    chkconnect();
    client.publish(my_topic_data, inputString.c_str(), true);
  }
}

int rssi_loop_cnt = 0;

void read_wifirss()
{
  // LÃ¤hetetÃ¤Ã¤n WiFi signaalin voimakkuus tieto MQTT serverille
  if(rssi_loop_cnt % 600 == 0)
  {
    long rssi = WiFi.RSSI();
    String sw = String(rssi);
    
    chkconnect();
    trace(sw.c_str());
    client.publish(my_topic_wifi, sw.c_str(), false);
    rssi_loop_cnt=0; 
  }
  rssi_loop_cnt++; 
}

void loop() {
  chkconnect();
  //read_wifirss();
  serialEvent();
  //delay(100);
}

