#include <SoftwareSerial.h>

#define TX_PIN 2
#define RX_PIN 3
#define RST    4
#define FLSH   5
#define PDSEL  6

  // put your setup code here, to run once:
SoftwareSerial espSerial(RX_PIN, TX_PIN); // RX, TX

void setup() {
   // Open serial communications and wait for port to open:
   Serial.begin(57600);
   while (!Serial) {
     ; // wait for serial port to connect. Needed for native USB port only
   }
   
   // set the data rate for the SoftwareSerial port
   espSerial.begin(57600);

   //ESP8266 flash mode setup
   pinMode(RST,       INPUT);
   pinMode(FLSH,      INPUT);
   pinMode(PDSEL,     INPUT);
   pinMode(13,        OUTPUT);
   digitalWrite(13,   LOW);
}
void esp_flash_mode(){
  
   digitalWrite(13,   HIGH);
   pinMode(RST,       OUTPUT);
   pinMode(FLSH,      OUTPUT);
   pinMode(PDSEL,     OUTPUT);
   digitalWrite(RST,  LOW);
   digitalWrite(FLSH, LOW);
   digitalWrite(PDSEL,HIGH);
   delay(500);
   digitalWrite(RST,  HIGH);
   delay(100);
   digitalWrite(FLSH, HIGH);
   pinMode(RST,       INPUT);
   pinMode(FLSH,      INPUT);
   pinMode(PDSEL,     INPUT);
   digitalWrite(13,   LOW);
}
volatile bool first_msg = true;

void loop() { // run over and over
   if(first_msg == true){
    esp_flash_mode();
    first_msg = false;
   }
   if (espSerial.available()) {
     Serial.write(espSerial.read());
   }
   if (Serial.available()) {
     espSerial.write(Serial.read());
   }
}
