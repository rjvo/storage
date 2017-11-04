// ILTO monitorointi ja ohjaus arduinolla

/* Signalointi:
    - 1. pari
      - Sininen         RX   --> VIHREA
      - Sini-Valkoinen  TX   --> VALKOINEN
    - 2. pari
      - Oranssi         +5V
      - Oranssi-Valkoi  gnd  --> MUSTA
    - 3. pari
      - Vihreä          gpio 3 (Output = Outcoing message - configure as input when not in use)
      - Vihreä-valkoin  gpio 2 (Input  = Incoming message IRQ - pullup resistor needed)
    - 4. pari
      - Ruskea          Reset  (Input - alhaalla aktiivinen - pullup resistor needed)
      - Ruskea-valkoin  gpio 4 (Output - Forse reset)
*/

// DHT22 sensorin kirjasto sekä tyyppi
#include <DHT.h>
#include "DHT.h"
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Servo kytketty signaaliin numero 10 (PWM ouput)
// Servon ohjaamiseen käytetään omaa kirjastoa
#include <Servo.h> 
Servo talteenottoservo;

// Servon ohjausarvot 0-9 on asetettu taulukkoon erikseen mittattujen arvojen pohjalta.
int talteenottotable[] = {1450, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2300, 2350};

// GPIO 9-10 varattu ohjaukuseen
#define LAMMITYSRELE 9
#define TALTEENOTTOSERVO 10

// Luodaan DHT luokat jokaiselle sensorille
// Korvaus, Jäte, Poisto , Ulko
// 8   , 11  , 12     , 13
// Alustetaan luokat DHT sensoreille
DHT dhta( 8, DHTTYPE);
DHT dhtb(11, DHTTYPE);
DHT dhtc(12, DHTTYPE);
DHT dhtd(13, DHTTYPE);

// UART puskurin määritykset
#define UART_BUFFER_LEN 32
#define UART_RX_PIN 2
volatile int  uart_data_len = 0;
volatile char uart_data[UART_BUFFER_LEN];

// Jos 
#define IM_ALIVE 10
int alive_cnt = IM_ALIVE;

void (*resetme)(void) = 0;

// Treisaus fuktio
void trace(char* txt)
{
  Serial.print("{\"TRACE\":\"");
  Serial.print(txt);
  Serial.println("\"}");
  Serial.flush();
}
// Treisuas fuktio
void trace(String txt)
{
  Serial.print("{\"TRACE\":\"");
  Serial.print(txt);
  Serial.println("\"}");
  Serial.flush();
}

// Yleinen järjestelmän alustus funktio
void setup() {
  // Lisäominaisuuksille varattuja signaaleja
  // pinMode(2, INPUT); Rerved for IRQ in (RX interrupt)
  // pinMode(3, INPUT); Reserved for TX starting int
  // pinMode(4, INPUT); Reserved for reset out
  
  // Kytketään srvo käyttöön
  talteenottoservo.attach(TALTEENOTTOSERVO, talteenottotable[0], talteenottotable[9]);
  
  // Asetetaan lämmitysrele päälle
  pinMode(LAMMITYSRELE, OUTPUT);
  digitalWrite(LAMMITYSRELE, 1);

  // Alustetaan sarjaportti
  Serial.begin(115200);
  
  trace("Setup");

  // Alustetaan DHT sensorit
  dhta.begin();
  dhtb.begin();
  dhtc.begin();
  dhtd.begin();

  // Annetaan servolle aikaa siirtyä paikoilleen ennenkö se pistetään pois päältä
  // Servot ovat poissa päältä ääriasennoissaan (0 ja 9 = täysin auki/kiinni)
  delay(2000);
  talteenottoservo.detach();
}

void read(DHT* asensor, float * result)
{
  // Luetaan lämpötila ja kosteus
  // tiedon lukeminen saattaa kestää jopa 250 millisekuntia.
  float h = asensor->readHumidity();
  float t = asensor->readTemperature(); // Celsius asteikolla
  
  // Varmistetaan että tieto saatiin luettua
  if (isnan(h) || isnan(t)) {
    h = -100;
    t = -100;
  }
  result[0] = t;
  result[1] = h;
}

// Sisääntulevan UART tiedon tarkistus ja luku puskuriin
void vastanotto()
{
  uart_data_len = 0;
  // Jos sisään on tullut dataa, niin luetaan se
  while(Serial.peek() > 0)
  {
    // Read UART
    uart_data[uart_data_len] = Serial.read();
    uart_data_len++;
  }
  // Trasetetaan luettu data
  if(uart_data_len>0)
  {
    trace((char*)uart_data); 
  }
  
  //Tyhjätään vileä varoiksi UART puskuri
  Serial.read();
}

// Käydään kaikki sensorit läpi ja muodostetaan JSON viesti, mikä
// läheteään UART:n yli.
void sensorit()
{
  trace("Reading sensors...");
  float result[4*2];
  read(&dhta, &result[0]);
  read(&dhtb, &result[2]);
  read(&dhtc, &result[4]);
  read(&dhtd, &result[6]);
  trace("...sensors read."); 
  delay(10);
  
  Serial.print("{\"DATA\":[");

  // Tarkistetaan arvot ja tulstetaan UART:n yli.
  for(int i=0; i < 8; i++)
  {
    float val = result[i];
    if(val != -100)
    {
      Serial.print(val);
    }else
    {
      Serial.print("null");
    }
    Serial.print(",");
  }
  
  // Loppuun vielä ylimääräinen parameteri ja loppu sulkeet
  // ja tulostus rivinvaihdolla.
  Serial.println("0]}");
  Serial.flush();
}

// Lämmitys releen ohjaus - yksinkertainen IO ohjaus (0 tai 1)
void lammitys(bool state)
{
  if(state == true)
  {
    trace("Lammitys paalle");
  }
  else
  {
    trace("Lammitys pois");  
  }
  digitalWrite(LAMMITYSRELE, state);
}

// Lämmöntalteenotto servon ohjaus ohjausarvolla 0-9
// Ääriasennoissa servo pistetään pois päältä (0 ja 9 = Auki ja kiinni)
void lammontalteenotto(int state)
{
  String t = "Lammontalteentotto ";
  t += state;
  trace(t);
  if(talteenottoservo.attached() == false)
  {
    talteenottoservo.attach(TALTEENOTTOSERVO);
  }
  
  if(state >= 0 && state < 10)
  {
    talteenottoservo.writeMicroseconds(talteenottotable[state]);     
  }

  if(state == 0 || state == 9)
  {
    // Ennen servon ohjauksen sammuttamista, annetaan servolle 2s aikaa
    // siirtyä uuteen sijaintiin.
    delay(2000);
    talteenottoservo.detach();
  }
}

// Ohjaus datan parsiminen, käytännössä luetaan UART puskurin 1. byte, mikä on ohjaus
// ja seuraava byte, mikä on ohjauksen parametri.
void ohjaus(){
  String lparam = "";
  if(uart_data_len > 1)
  {
  switch( uart_data[0] ){
    case 'T':
      // lämmöntalteenoton ohjaus
      lparam += (char)uart_data[1];
      lammontalteenotto((int)lparam.toInt());
      break;
    case 'L':
      // lämpötilareleen ohjaus
      if(uart_data[1] == '1')
      {
        lammitys(true);  
      }
      else
      {
        lammitys(false);
      }
      break;
    case 'P':
      alive_cnt = IM_ALIVE;
      trace("ESP-01_Pong");
      break;
    case 'R':
      Serial.flush();
      resetme();
    case 'G':
      trace("G");
      sensorit();
      break;
    }
  }
  // Tyhjätään UART puskuri
  memset((void*)uart_data,0,UART_BUFFER_LEN);
  uart_data_len = 0;
}

// Main loop funktio
int loopi = 3000;

void loop() {
  // looppin joka 300. kerta (300 * 100ms) eli noin 30s luetaan sensoreiden tiedot. 
  if(loopi >= 300){
    sensorit();
    loopi = 0;
    alive_cnt--;
    if(alive_cnt==4)
    {
      trace("Goigng to reset the system in 2mins...");
    }
    if(alive_cnt==1)
    {
      trace("Goigng to reset the system in 30s...");
    }
    if(alive_cnt==0)
    {
      Serial.print("RRRR");
      Serial.flush();
      delay(100);
      resetme();
    }
  }
  loopi++;

  // Joka kierroksella luetaan UART puskuri ja tehdään halutut ohjaukset
  vastanotto();
  ohjaus();

  // 100ms viive jokaisella kierroksella.
  delay(100);
}
