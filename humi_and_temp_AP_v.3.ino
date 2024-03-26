/* 
używa elegant ota - for OTA updates: copy the IPAddress displayed over your Serial Monitor 
and go to http://<IPAddress>/update in browser. 
pokazuje wilgotność i temperaturę 

- strona web - zrobić
- oled - zrobić
- wrzuca w google sheets - zrobić

Napisane dla wemos d1 mini lite(ESP8266) ale powinno działać z ESP32
d1 mini for manual programing d3 -> ground +reset


*/

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <ESP8266WebServer.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <WebServer.h>
#endif

#include <ElegantOTA.h>
#include <DHT.h>
#define DHT11PIN 13
#define DHTTYPE DHT11

#if defined(ESP8266)
  ESP8266WebServer server(80);
#elif defined(ESP32)
  WebServer server(80);
#endif

const char* ssid = "ESP8266";
const char* password = "qwertyuiop";

const int DHTPin = 13;
DHT dht(DHTPin, DHTTYPE);


float h;
float t;
float hic;

static char celsiusTemp[7];
static char humidityTemp[7];



void handleRoot() {

//Tu budujemy co mamy wysłać:
  String webpage = "<!DOCTYPE html><html>\
  <head><meta http-equiv='refresh' content='60'/>\
    <link rel=\'icon\' href=\'data:,\'>\
    <title>TEMPERATURA I WILGOTNOSC</title>\
  </head>\
  <BODY bgcolor='#333366'>\
    <TABLE border='5' bordercolor='#ffffff' width='600px' cellpadding='5' align='center' cellspacing='5'>\
      <TBODY>\
        <TR bgcolor='#ffffff'>\
          <TD align='center' colspan='5'>\
            <FONT color='#993300'>\
              <h1> TEMPERATURA I WILGOTNOSC </h1>\
            </FONT>\
          </TD>\
        </TR>\
        <TR bgcolor='#ffffff'>\
          <TD align='center'>\
            <h1>Temperatura</h1>\
          </TD>\
          <TD align='center'>\
            <h1>";
  webpage += String(t) + " °" + "</h1> </TD></TR>";
  webpage += "<TR bgcolor='#ffffff'> <TD align='center'> <h1>Temperatura odczuwalna</h1> </TD> <TD align='center'> <h1>";
  webpage += String(hic) + " °" + "</h1> </TD> </TR>";
  webpage += "<TR bgcolor='#ffffff'> <TD align='center'> <h1>Wilgotnosc</h1> </TD> <TD align='center'> <h1> ";
  webpage += String(h) + " %" + "</h1> </TD> </TR> </TBODY> </TABLE> </BODY> </html>";
  
  server.send(200, "text/html", webpage);
}
 


void setup() {
  
  Serial.begin(115200);
  dht.begin();
 
  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.print("Server IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Server MAC address: ");
  Serial.println(WiFi.softAPmacAddress());  
  
 
  
  /*
  server.on("/", []() {
    server.send(200, "text/plain", "Hi! I am ESP8266.");
  });
  */


  ElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  server.on("/", handleRoot); 
  Serial.println("Server listening");

 

}

void loop() {
 
  server.handleClient();
  
  h = dht.readHumidity();
  t = dht.readTemperature();
  hic = dht.computeHeatIndex(t, h, false);
  dtostrf(hic, 6, 2, celsiusTemp);
  dtostrf(h, 6, 2, humidityTemp);

  Serial.print(t);
  Serial.print(" - ");
  Serial.println(h);


}
