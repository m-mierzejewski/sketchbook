/*
  Pomiar szamba i webserwer na ESP8266 12E
  Kompilować dla płytki NodeMCU 0.9

  Działa:
  - serwer web
  - diody LED na PWM1
  - blinker
  - Google docs zrobione na telewizormarcina.
  - wyświetlacz I2C
  - wyświetla "Net" jak jest połączony z wifi
  - startuje bez problemów po resecie
  - publikowany pomiar to średnia 3 ostatnich fizycznych pomiarów
  - (nowe!) wysyłanie maili - wypróbować.
  - (nowe!) OTA firmware updates - port: Szambo_W50e
  - reset co parę dni
  - weryfikacja przeciwko szybkim przyrostom

  LED: czerwona miga jak probuje sie-ę łączyć z WIFI, Trzy zielone
  błyski po udanym połączeniu. Czer-ziel błyski po wysłaniu strony na
  przeglądarkę.

  Wysyła na pushing box 2 parametry &distance i &procent - devid
  Pushing box opakowuje to w https i przesyła na Googledocs

  Wysyła maile przy 10, 5 i 3 cm - wysyła do pushingboxa - devid1

  Uwaga !!! ten szkic jest z konwerterem poziomów logicznych.
  ESP8266 ma 3v3 a ultradźwiękowy czujnik 5.0V !!!!

  Użyto pinu D4 zamiast D8 do Triger-a czujnika co rozwiązało problem
  niestartującego ESP 8266 po resecie.


  flashowanie kablem: mostek GND + D3
*/


#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoOTA.h>

                        //D1 - SCL do I2C
                        //D2 - SDA do I2C
#define trigPin D4      //D4 - trigger czujnika
#define RledPin D5      //D5 - czerwony LED
#define GledPin D6      //D6 - zielony LED
#define echoPin D7      //D7 - echo czujnika
                        //D8 - nieużywany builtin led?


const char *ssid1 = "Torba"; //nazwa sieci do której ma się podłączyć
const char *ssid2 = "torba";
const char *password1 = "qwertyuiop"; // hasło wifi 1
const char *password2 = "qwertyuiop"; // hasło wifi 2
const char *otapassword = "12345"; // hasło do flashowania przez OTA
const char WEBSITE[] = "api.pushingbox.com"; //pushingbox API server
const String devid = "v9E0DD9C98A61BAC"; //device ID - Pushingbox dla google sheets
const String devid1 = "vEDE52E16577C664"; //device ID - Pushingbox dla email 

const int lowlimit = 300;   // tu światło zaczyna się robić pomarańczowe
const int highlimit = 30;   // tu światło jest czerwone
const int wysokosc = 1307;  // całkowita wysokość szamba w mm
const int czestosc = 30 * 1000; // okres pomiaru w ms
const int gczest = 10 * 60 * 1000; //co 10 min do google
const int kalibracja = 385;  // zero o tyle przed dotknięciem sensora (385 - dla naszego szamba)
const int jasno = 1023;       // max jasność ledów - maks cykl PWM (max 1023 = 100% cykl), 1023 - dla tranzystorów.
const int resetDays = 1;  // liczba ddni między rewsetami

int procent;
int distance;
int odl0;
int odl1;
int odl2;
int odl3;
long starttime;
long timer;
long googletime;
long duration;
int gbright;
int rbright;
int k;
int segmenty;
boolean pierwszy_pomiar, a10, a5, a3;


ESP8266WebServer server(80);
LiquidCrystal_I2C lcd(0x3f, 16, 2); // set the LCD address to 0x3f / 0x27 for a 16 chars and 2 line display

void display(int toDisplay)   //wyświetla na wyświetlaczu I2C
{

  lcd.init(); // lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(toDisplay);
  lcd.print("mm");
  lcd.setCursor(12, 0);
  lcd.print(procent);
  lcd.print("%");
  if (WiFi.status() == WL_CONNECTED)
  {
    lcd.setCursor(7, 0);
    lcd.print("Net");
  }
  if (toDisplay < 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("ALARM - PELNE!");
  }
  else if (toDisplay > wysokosc + 50)
  {
    lcd.setCursor(0, 1);
    lcd.print("Hmm! Co jest?");
  }
  else
  {
    segmenty = 12 - map(toDisplay, 0, wysokosc, 0, 12);
    procent = map(toDisplay, 0, wysokosc, 0, 100);

    lcd.setCursor(0, 1);
    lcd.print("0");
    for (int i = 0; i < segmenty; i++)
    {
      lcd.print("#");
    }
    for (int i = segmenty; i < 13; i++)
    {
      lcd.print("_");
    }
    lcd.setCursor(13, 1);
    lcd.print("100");
  }

}


void handleRoot()

/* tu budujemy wyświetlaną stronę */
{
  String webpage = "<!DOCTYPE html><html>\
  <head><meta http-equiv='refresh' content='60'/>\
    <link rel=\'icon\' href=\'data:,\'>\
    <title>SZAMBO</title>\
  </head>\
  <BODY bgcolor='#333366'>\
    <TABLE border='5' bordercolor='#ffffff' width='600px' cellpadding='5' align='center' cellspacing='5'>\
      <TBODY>\
        <TR bgcolor='#ffffff'>\
          <TD align='center' colspan='5'>\
            <FONT color='#993300'>\
              <h1> SZAMBO </h1>\
            </FONT>\
          </TD>\
        </TR>\
        <TR bgcolor='#ffffff'>\
          <TD align='center'>\
            <h1>Wolna wysokosc</h1>\
          </TD>\
          <TD align='center'>\
            <h1>";

  webpage += String(distance) + " mm" + "</h1> </TD></TR>";
  if (distance < 0)
  {
    webpage += "<TR bgcolor='#ffffff'> <TD align='center' colspan='2'><FONT color='#ff0000'>\
        <h1>Natychmiast wybierz szambo !!!</FONT></TD> </TR> </TBODY> </TABLE> </BODY> </html>";
  }
  else if (distance > wysokosc)
  {
    webpage += "<TR bgcolor='#ffffff'> <TD align='center' colspan='2'><FONT color='#ff0000'>\
        <h1>Chyba cos nie tak z pomiarem ?!?!?</FONT></TD> </TR> </TBODY> </TABLE> </BODY> </html>";
  }
  else
  {
    webpage += "<TR bgcolor='#ffffff'> <TD align='center'> <h1>Zostala objetosc</h1> </TD> <TD align='center'> <h1>";
    webpage += String(7.3124 * distance / 1000) + " m3" + "</h1> </TD> </TR>";
    webpage += "<TR bgcolor='#ffffff'> <TD align='center'> <h1>Wolne</h1> </TD> <TD align='center'> <h1> ";
    webpage += String(procent) + " %" + "</h1> </TD> </TR> </TBODY> </TABLE> </BODY> </html>";
  }
  server.send(200, "text/html", webpage);

  // Pomrugaj po wysłaniu strony
  analogWrite(RledPin, 0);
  analogWrite(GledPin, 0);
  delay(50);
  analogWrite(RledPin, jasno);
  analogWrite(GledPin, 0);
  delay(50);
  analogWrite(RledPin, 0);
  analogWrite(GledPin, jasno);
  delay(50);
  analogWrite(RledPin, jasno);
  analogWrite(GledPin, 0);
  delay(50);
  analogWrite(RledPin, 0);
  analogWrite(GledPin, jasno);
  delay(50);
  analogWrite(RledPin, jasno);
  analogWrite(GledPin, 0);
  delay(50);
  analogWrite(RledPin, 0);
  analogWrite(GledPin, 0);
  delay(50);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void connectwifi()     // usiłuje połączyć się z wifi przez 30 s.
{
  if (WiFi.status() != WL_CONNECTED)
  {
    timer = millis();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid1, password1);
    Serial.println("Zaczynamy laczyc WIFI");
    Serial.println(ssid1);
    while ((WiFi.status() != WL_CONNECTED) && ( millis() - timer < 30000)) // Wait for connection 30 s
    {
      analogWrite(RledPin, jasno);    // czerwone krótkie błyski przy braku połączenia
      delay(30);
      Serial.print(".");
      analogWrite(RledPin, 0);
      delay(470);
    }
    Serial.println("");
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    timer = millis();
    WiFi.begin(ssid2, password2);
    Serial.println(ssid2);
    while ((WiFi.status() != WL_CONNECTED) && ( millis() - timer < 30000)) // Wait for connection 30 s
    {
      analogWrite(RledPin, jasno);    // czerwone krótkie błyski przy braku połączenia
      delay(30);
      Serial.print(".");
      analogWrite(RledPin, 0);
      delay(470);
    }
    Serial.println("");
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.print("Connected to WIFI:");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    analogWrite(GledPin, 0);          //po połączeniu 3 zielone błyski
    delay(400);
    analogWrite(GledPin, jasno);
    delay(200);
    analogWrite(GledPin, 0);
    delay(200);
    analogWrite(GledPin, jasno);
    delay(200);
    analogWrite(GledPin, 0);
    delay(200);
    analogWrite(GledPin, jasno);
    delay(200);
    analogWrite(GledPin, 0);
    if (MDNS.begin("szambo"))
    {
      Serial.println("MDNS responder started");
    }
  }
  else
  {
    Serial.println("NOT CONNECTED WIFI!!!");
    Serial.println("");
  }
}

void sendToGoogle()       // wysyła dane do googledocs przez pushingbox
{
  WiFiClient client;  //Instantiate WiFi object
  if (client.connect(WEBSITE, 80))
  {

    client.print("GET /pushingbox?devid=" + devid
                 + "&distance=" + (String) distance
                 + "&procent="      + (String) procent
                );

    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(WEBSITE);
    client.println("User-Agent: ESP8266/1.0");
    client.println("Connection: close");
    client.println();
  }
  Serial.println("próba wysłania do Google");
}

void sendemail(int odl)
{
  WiFiClient client;  //Instantiate WiFi object
  if (client.connect(WEBSITE, 80))
  {
    client.print("GET /pushingbox?devid=" + devid1 + "&brakuje=" + (String) odl );
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(WEBSITE);
    client.println("User-Agent: ESP8266/1.0");
    client.println("Connection: close");
    client.println();
  }
  Serial.println("próba wysłania maila");
}

void setup()
{
  ArduinoOTA.begin();  // ustawianie parametrów do OTA
  ArduinoOTA.setPassword(otapassword); // hasło do flashowania przez sieć
  ArduinoOTA.setHostname("Szambo_W50e"); //tak się będzie nazywał port sieciowy.
  
  a10 = true; // poziomy alarmu do maila
  a5 = true;
  a3 = true;
  analogWrite(RledPin, 0); //na dobry początek
  analogWrite(GledPin, 0);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  starttime = 0;
  googletime = millis();
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  server.on("/", handleRoot);
  server.on("/inline", []()
  {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  connectwifi();
  pierwszy_pomiar = 1;
}

void loop()
{
  ArduinoOTA.handle();
  if (millis() > starttime) // czas mierzyć
  {
    Serial.print(millis());
    Serial.print(" -> ");

    digitalWrite(trigPin, LOW);  // tu sie pinguje
    delayMicroseconds(5);         //
    digitalWrite(trigPin, HIGH);   //
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    odl0 = ((duration / 2) / 2.91) - kalibracja;
    if (pierwszy_pomiar == 1)
    {
      odl1 = odl0;
      odl2 = odl0;
      odl3 = odl0;
      pierwszy_pomiar = 0;
      distance = (odl1 + odl2 + odl3) / 3; // średnia z 3 pomiarów
    }
    else
    {
      if(odl0>(distance-100)) //unikanie dużych przyrostów-błąd pomiaru(100 mm)
      {
        odl3 = odl2;
        odl2 = odl1;
        odl1 = odl0;
        distance = (odl1 + odl2 + odl3) / 3; // średnia z 3 pomiarów
      }
    }
    procent = distance * 100 / wysokosc;
    Serial.print(duration);
    Serial.print("ms ");
    Serial.print(distance);
    Serial.print(" mm  - proc  -> ");
    Serial.println(procent);
    
    Serial.print("a10 = ");
    Serial.println(a10);


    Serial.print("a5 = ");
    Serial.println(a5);


    Serial.print("a3 = ");
    Serial.println(a3);
    
    starttime = millis() + czestosc; // następny pomiar
    display(distance); //wyświetla wynik na wyświetlaczu
  }
  if ((millis() - googletime) > gczest ) // czas wrzucić na serwer
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      analogWrite(RledPin, 0);
      analogWrite(GledPin, 0);
      delay(50);
      analogWrite(RledPin, jasno);
      analogWrite(GledPin, 0);
      delay(200);
      analogWrite(RledPin, 0);
      analogWrite(GledPin, 0);
      delay(50);
      Serial.println("Niepołączony - próbujemy!");
      connectwifi();
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      analogWrite(RledPin, 0);
      analogWrite(GledPin, 0);
      delay(50);
      analogWrite(RledPin, 0);
      analogWrite(GledPin, jasno);
      delay(200);
      analogWrite(RledPin, 0);
      analogWrite(GledPin, 0);
      delay(50);
      Serial.print("Połączony. Lokalny IP: ");
      Serial.println(WiFi.localIP());
      sendToGoogle();
    }
    else
    {
      Serial.println("");
      Serial.println("Nadal niepolaczony. Nic nie wysylam ");
    }
    googletime = millis();
  }
  // resetowanie co parę dni
  if (millis()>(resetDays*24*3600*1000)) ESP.restart();
   

  // Wysyłamy maile przez pushingbox
  if ((a10) and distance < 100)  // 10 cm do końca
  {
    sendemail(10);
    a10 = false; // mail wysłany alarm rozbrojony
  }
  if ((a5) and distance < 50)  // 5 cm do kończ
  {
    sendemail(5);
    a5 = false; // mail wysłany alarm rozbrojony
  }
  if ((a3) and distance < highlimit)  // led czerwony
  {
    sendemail(3);
    a3 = false; // mail wysłany alarm rozbrojony
  }

  // reset powiadomień mailem - jak pozion spadnie do zielonego leda to
  // ponownie uzbrajamy powiadomienia.
  if (distance > lowlimit)
  {
    a10 = true;
    a5 = true;
    a3 = true;
  }


  // (* Obsługa sygnalizacyjnego 2 kolorowego LED-a */

  if (distance < highlimit)  // poziom alarmowy - czerwone światło
  {
    rbright = jasno;
    gbright = 0;
  }
  if (distance < lowlimit && distance > highlimit) //mniej lub bardziej pomarańczowe
  {
    gbright = map(distance - highlimit, highlimit, lowlimit, 20, jasno);
    rbright = jasno - gbright;
  }
  if (distance >= lowlimit) // Zielone światło
  {
    rbright = 0;
    gbright = jasno;
  }
  analogWrite(RledPin, rbright);
  analogWrite(GledPin, gbright);

  server.handleClient();
}
