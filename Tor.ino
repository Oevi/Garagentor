// You have to adjust the network settings and eventually the Pins

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <user_interface.h>
#include <gpio.h>

// PIN Settings
#define PINTOR 4      // pin for door-sensor
#define PINHIGH 16    // I could not use VCC and ground pins, 
#define PINLOW 5      // so I made new pins high and low
#define OFFEN 1       // you might need to change OFFEN (open) and GESCHLOSSEN (closed)
#define GESCHLOSSEN 0 // dependent on your sensor

//Variablen
int Status = GESCHLOSSEN; // GESCHLOSSEN means closed
int StatusAlt = GESCHLOSSEN;
int wert = 0;
int httpCode = 0;
int i = 0;
String befehlZu = "Aus";
String befehlAuf = "Ein";
IPAddress ip(192, 168, 178, 81);  // adjust these IPs
IPAddress fritz(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);
const char* user = "LoxoneUser"; // user and password in Loxone
const char* pw = "LoxonePW";


// Network Settings
const char* ssid = "WIFI-AP-SSID";   // ssid of your wifi-ap
const char* password = "WIFI-PW";

//Are we currently connected?
boolean verbunden = false;

// HTTP Client 
HTTPClient http;

// Notwendig!
WiFiClient wclient;

void setup() {
  // put your setup code here, to run once:

  // initiate serial output (only for debugging)
  Serial.begin(115200);
  delay(10);
  Serial.println();

  //WLAN IP-Adresse statisch festlegen
  WiFi.config(ip, fritz, subnet, fritz);
  Serial.println("IP-Adresse nach config: " + WiFi.localIP().toString());

  // Set pin modes
  gpio_init();
  pinMode(PINHIGH,OUTPUT);
  digitalWrite(PINHIGH,HIGH);
  pinMode(PINLOW,OUTPUT);
  digitalWrite(PINLOW,LOW);
  pinMode(PINTOR,INPUT_PULLUP);
  
}

void loop() {
  // Start wifi an connect
  WlanAn();

  // Read the door-sensor
  Status = digitalRead(PINTOR);
  if (Status == OFFEN)
  {
    if (sendHTTP(befehlAuf))  // send the status (open) to Loxone
    {
      StatusAlt = OFFEN;
    }
    Serial.println("Tor geöffnet.");
  }
  else
  {
    if (sendHTTP(befehlZu)) // send the status (closed) to Loxone
    {
      StatusAlt = GESCHLOSSEN;
    }
    Serial.println("Tor geschlossen.");
  }
  schlafen(Status);   // set sleep-mode
  
  delay(200);         // sleeping starts with this delay-command (untill door-status changes, not only 200 ms)

  i += 1;           // only for debugging
  Serial.print("Schleife ");
  Serial.println(i);
}

// function, to send status to loxone
boolean sendHTTP(String text)
{
  if(verbunden)
  {
    String url = "http://192.168.178.5:5100/dev/sps/io/Garagentor/";  // url depends on loxone-IP, port and virual http input name
    url.concat(text);
    http.begin(wclient, url);
    http.setAuthorization(user, pw);
    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      Serial.println("Befehl " + text + " gesendet.");
      http.end();
      delay(10);
      wclient.stop();
      return true;
    }
    http.end();
    wclient.stop();
    delay(10);
  }
  return false;
}

// function to set sleep-mode
void schlafen(int Status)
{
  Serial.println("Gehe schlafen.");
  wifi_station_disconnect();
  verbunden = false;
  wifi_set_opmode_current(NULL_MODE);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_fpm_open();
  if (Status == 1)
  {
    gpio_pin_wakeup_enable(GPIO_ID_PIN(PINTOR), GPIO_PIN_INTR_LOLEVEL);
  }
  else
  {
    gpio_pin_wakeup_enable(GPIO_ID_PIN(PINTOR), GPIO_PIN_INTR_HILEVEL);
  }
  wifi_fpm_do_sleep(0xFFFFFFF);
  Serial.println("Ende der Schlafen-Schleife.");
  
}

// function to start wifi
void WlanAn()
{
  // start WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password);
  Serial.print("Verbinde zu Netzwerk ");
  Serial.print(ssid);
  Serial.print(" ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Verbindung hergestellt.");
  Serial.println("IP-Adresse: " + WiFi.localIP().toString());
  verbunden = true;
}
