
#define MQTT_KEEPALIVE 60 // Tarvitaan että MQTT yhteys pysyy ongelmitta pystyssä

#include <ESP8266WiFi.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>

//Wi-Fi verkon parametrit MUUTA
#define wifi_ssid "myIOT"
#define wifi_password "internetofthings"

//MQTT serverin osoite MUUTA
#define mqtt_server "192.168.0.203"

//Tämmä täytyy olla kaikille MQTT laitteille yksilöllinen!
#define MQTT_CLIENT_ID "IoTProjektiIlto"

//MQTT aiheet
#define my_topic_data   "ilto/data"
#define my_topic_ctrl   "ilto"
#define my_topic_state  "ilto/state"
#define my_topic_wifi   "ilto/wifi/main"
#define my_topic_trace  "ilto/trace"

//UART puskurin koko
#define MAX_UART_CHARS 200
#define ILTO_MSG_MAX_LEN 10

//Sisääntulevien MQTT viestien callback fuktio
void msg_callback(const char* topic, byte* payload, unsigned int len);

volatile char uart_data[MAX_UART_CHARS];
int uart_data_len = 0;

//Luodaan WiFiClient ja MQTT PubSubClient luokat
WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, msg_callback, espClient);

//RnD käyttöön debug tulostukset UART kanavaan
#define tracing false
void trace(const char * text)
{
  if(tracing == true)
  {
    Serial.println(text);
  }
}

//Wi-Fi asetukset
void setup_wifi() {
  // We start by connecting to a WiFi network
  if(WiFi.status() != WL_CONNECTED)
  {
    trace("Connecting to wifi");
  
    WiFi.begin(wifi_ssid, wifi_password);
  
    while (WiFi.status() != WL_CONNECTED) {
      //Odotetaan 0.5s ennenkö yritetään uudelleen
      delay(500);
      trace(".");
      Serial.print("Q:");
    }
    trace("WiFi connected");
    //WiFi.localIP()
    Serial.print("QW");
  }
}

//Tilattuun MQTT aiheeseen tulevat viestit päätyvät tänne, 
//mistä ne lähetetään UART porttiin.
void msg_callback(const char * topic, byte* payload, unsigned int len) {
  int i = 0;
  for (i = 0; i < len; i++) {
    if(i < ILTO_MSG_MAX_LEN)
    {
      Serial.print((char)payload[i]);
    }
  }
 //Serial.println();
  Serial.flush();
  /*
  if((char)payload[0] == 'R')
  {
    ESP.reset();
  }
  */
}

//Tarkistetaan yhteys ja jos yhteyttä ei ole, niin luodaan sellainen (Wi-Fi + MQTT)  
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

//Arduinon alustus funktio
void setup() {
  Serial.begin(115200);
  while(Serial.peek()>0)
  {
    Serial.read();
  }
  Serial.print("Q*");
  setup_wifi();
  delay(1000);
  chkconnect();
}

//Luetaan UART data
void serialEvent() {
  
  if (Serial.peek() > 0)
  {
    char merkki = 0;
    if(uart_data_len == 0)
    {
      memset((void*)uart_data,0,sizeof(uart_data));
    }
    // Jos sisään on tullut dataa, niin luetaan se

    while(Serial.peek() > 0 && merkki != '}' && uart_data_len < MAX_UART_CHARS)
    {
      // Read UART
      merkki = Serial.read();
      if(merkki != '\n')
      {
        uart_data[uart_data_len] = merkki; 
        uart_data_len++;
      }
    }
    if(merkki == '}' || uart_data_len > MAX_UART_CHARS-2)
    {
      chkconnect();
      client.publish(my_topic_data, (char*)uart_data, true);
      uart_data_len = 0;
    }
    if(uart_data_len > 12 && uart_data[10] == 'R' && uart_data[11] == 'R')
    {
      ESP.reset();
    }
  }
  /*
    inputString = Serial.readString();
    trace(inputString.c_str());
    if(inputString.length() > 12 && inputString.charAt(10) == 'R' && inputString.charAt(11) == 'R')
    {
      ESP.reset();
    } else 
    {
      chkconnect();
      client.publish(my_topic_data, inputString.c_str(), true);
    }
  }
    String inputString = "";         // a string to hold incoming data
  inputString.reserve(MAX_UART_CHARS);
  */
}

//Arduino main loop
void loop() {
  chkconnect();
  serialEvent();
}
