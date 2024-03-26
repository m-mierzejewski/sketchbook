/*
 * Dziala
 * - WIFI client
 * - pamieć pomiedzy wylączeniami
 * - uchylone zawory 
 * 
 * Gdyby wychodził błąd przy kompilacji że brak PYSERIAL
 * to trzeba go zainstalować do Pythona za pomocą 
 * "python setup.py install" - nie za pomocą "pip"
 * 
 * przy pierwszym uruchamianiu płytki usunąć znaki komentaża z 
 * linii 70-79, wgrać. Potem wstawić komentaże z powrotem  
 * i wgrać jeszcze raz - inicjowanie pamięci
 * 
 * Zrobione na ESP32-WROOM-32U
 */

// Load library
#include <WiFi.h>
//#include <WiFiClient.h>
#include <EEPROM.h>

// Replace with your network credentials
const char* ssid     = "W50e";
const char* password = "qwertyuiop";
const int chanel = 10;
const int ssid_hidden = 1;
const int max_connection = 2;

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String CWSt; 
String ZWSt; 
String PRSt1;
String PRSt2;
String PRSt3;


// Assign output variables to GPIO pins
const int CWO = 27; // otwórz CW 
const int CWZ = 26; // zamknij CW
const int ZWO = 25; // otwórz ZW 
const int ZWZ = 33; // zamknij ZW
const int PRZ1 = 16; // Przelącz prąd L1
const int PRZ2 = 17; // Przelącz prąd L2
const int PRZ3 = 5; // Przelącz prąd L3

// Current time
unsigned long currentTime = millis();

// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

char mem[5] ="";


void setup() {
  EEPROM.begin(128);
  Serial.begin(115200);

// pierwsze uruchomienie - na nowej płytce trzeba najpierw zainicjalizować pamięć 
// - początkowe wartości. Usunąć znaki komentarza (/* ... */), wgrać, spowrotem zaznaczyć 
// znaki komentarza i wgrać jeszcze raz

/*    
  mem[0]=120;
  mem[1]=120;
  mem[2]=120;
  mem[3]=120;
  mem[4]=120;
  EEPROM.put(5,mem);
  EEPROM.commit();
  EEPROM.get(5,mem);
*/

  EEPROM.get(5,mem); // load last configuration 
  Serial.println();
  Serial.println(mem[0]);
  Serial.println(mem[1]);
  Serial.println(mem[2]);
  Serial.println(mem[3]);
  Serial.println(mem[4]);
  
// Initialize the output variables as outputs
  pinMode(CWO, OUTPUT); // otwieranie CW
  pinMode(CWZ, OUTPUT); // zamykanie CW
  pinMode(ZWO, OUTPUT); // otwieranie ZW
  pinMode(ZWZ, OUTPUT); // zamykanie ZW
  pinMode(PRZ1, OUTPUT); // przełączanie prądu L1
  pinMode(PRZ2, OUTPUT); // przełączanie prądu L2
  pinMode(PRZ3, OUTPUT); // przełączanie prądu L3
 
// Establish AP
  Serial.println();
  Serial.print("Configuring access point...");
  //You can remove the password parameter if you want the AP to be open. 
  WiFi.softAP(ssid, password, chanel , ssid_hidden, max_connection); //pracuje na 10 kanale, false - otwartaarta , max 1 polączenia

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
   
  //server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
  
  // Set outputs as per EEPROM readouts - last configuration

  // CIEPLA WODA
  
  if(mem[0]==120){        //x - (ascii 120) - wylączony
      CWSt="zamknieta";
      Zamknij("CW");
    } 
   
  else if(mem[0]==112){        //p - (ascii 112) - wylączony
    CWSt="uchylona";
    Przymknij("CW");
    } 

   
  else {
    CWSt="otwarta";
    Otworz("CW");
    } 
  

//ZIMNA WODA
  
    if(mem[1]==120){        //x - (ascii 120) - zamknieta
      ZWSt="zamknieta";
      Zamknij("ZW");
    } 
   
    else if(mem[1]==112) {        //p - (ascii 112) - przymknij
      ZWSt="uchylona";
      Przymknij("ZW");
    } 
  
    else {
      ZWSt="otwarta";           // otwarta
      Otworz("ZW");
    } 

//PRAD
// L1

  if(mem[2]==120){        //x - (ascii 120) - wylączony
    PRSt1="wylaczony";
  }
  else{
    PRSt1="zalaczony";
  }

// L2

  if(mem[3]==120){        //x - (ascii 120) - wylączony
    PRSt2="wylaczony";
  }
  else{
    PRSt2="zalaczony";
  }

// L3

  if(mem[4]==120){        //x - (ascii 120) - wylączony
    PRSt3="wylaczony";
  }
  else{
    PRSt3="zalaczony";
  }

  
   Serial.println("Init READY!");
}
  


void loop(){

   
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // otwiera i zamyka zawory / prad
            
            if (header.indexOf("GET /CW/on") >= 0) {
              Serial.println("otwieram");
              CWSt = "otwarta";
              Otworz("CW");
              mem[0]=111;                                 // o - (ASCII 111) - wlączony 
              Serial.println("CW otwarta");
              } 
              else if (header.indexOf("GET /CW/off") >= 0) {
              Serial.println("zamykam");
              CWSt = "zamknieta";
              Zamknij("CW");
              mem[0]=120;                                 // x - (ascii 120) - wylączony
              Serial.println("CW zamknieta");
              } 
              else if (header.indexOf("GET /CW/part") >= 0) {
              Serial.println("uchylam");
              CWSt = "uchylona";
              Przymknij("CW");
              mem[0]=112;                                 // x - (ascii 110) - uchylony
              Serial.println("CW uchylona");
              } 
            
              else if (header.indexOf("GET /ZW/on") >= 0) {
              Serial.println("otwieram");
              ZWSt = "otwarta";
              Otworz("ZW");
              mem[1]=111;                                 // o - (ASCII 111) - wlączony
              Serial.println("ZW otwarta");
              } 
              else if (header.indexOf("GET /ZW/off") >= 0) {
              Serial.println("zamykam");
              ZWSt = "zamknieta";
              Zamknij("ZW");
              mem[1]=120;                                 // x - (ascii 120) - wylączony
              Serial.println("ZW zamknieta");
                            }
              else if (header.indexOf("GET /ZW/part") >= 0) {
              Serial.println("uchylam");
              ZWSt = "uchylona";
              Przymknij("ZW");
              mem[1]=112;                                 // x - (ascii 110) - uchylony
              Serial.println("ZW uchylona");
              } 
              else if (header.indexOf("GET /PR1/on") >= 0) {
                Serial.println("Zalaczam");
                PRSt1 = "zalaczony";
                digitalWrite(PRZ1,HIGH);
                delay(200);
                digitalWrite(PRZ1,LOW);
                mem[2]=111;                         // o - (ascii 111) - zalaczony
                Serial.println("Prad L1 Zalaczony");
                }
              else if (header.indexOf("GET /PR1/off") >= 0) {
                Serial.println("Wylaczam");
                PRSt1 = "wylaczony";
                digitalWrite(PRZ1,HIGH);
                delay(200);
                digitalWrite(PRZ1,LOW);
                mem[2]=120;                                 // x - (ascii 120) - wylaczony
                Serial.println("Prad L1 Wylaczony");
                }

              else if (header.indexOf("GET /PR2/on") >= 0) {
                Serial.println("Zalaczam");
                PRSt2 = "zalaczony";
                digitalWrite(PRZ2,HIGH);
                delay(200);
                digitalWrite(PRZ2,LOW);
                mem[3]=111;                         // o - (ascii 111) - zalaczony
                Serial.println("Prad L2 Zalaczony");
                }
              else if (header.indexOf("GET /PR2/off") >= 0) {
                Serial.println("Wylaczam");
                PRSt2 = "wylaczony";
                digitalWrite(PRZ2,HIGH);
                delay(200);
                digitalWrite(PRZ2,LOW);
                mem[3]=120;                                 // x - (ascii 120) - wylaczony
                Serial.println("Prad L2 Wylaczony");
                }

              else if (header.indexOf("GET /PR3/on") >= 0) {
                Serial.println("Zalaczam");
                PRSt3 = "zalaczony";
                digitalWrite(PRZ3,HIGH);
                delay(200);
                digitalWrite(PRZ3,LOW);
                mem[4]=111;                         // o - (ascii 111) - zalaczony
                Serial.println("Prad L3 Zalaczony");
                }
              else if (header.indexOf("GET /PR3/off") >= 0) {
                Serial.println("Wylaczam");
                PRSt3 = "wylaczony";
                digitalWrite(PRZ3,HIGH);
                delay(200);
                digitalWrite(PRZ3,LOW);
                mem[4]=120;                                 // x - (ascii 120) - wylaczony
                Serial.println("Prad L3 Wylaczony");
                }
              
            EEPROM.put(5,mem);
            EEPROM.commit();
            EEPROM.get(5,mem);
            Serial.println(mem[0]);
            Serial.println(mem[1]);
            Serial.println(mem[2]);
            Serial.println(mem[3]);
            Serial.println(mem[4]);
            
                      
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            
            client.println(".button { background-color: #008000; border: none; color: white; padding: 6px 10px;");
            client.println("text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #FF0000;border: none; color: white; padding: 6px 10px;}");
            client.println(".button3 {background-color: #FFA500;border: none; color: white; padding: 6px 10px;}</style></head>");
            
            
            // Web Page Heading
            client.println("<body><h1>KONTROLA MEDIOW</h1>");
            
            // Display current state, and ON/OFF buttons for CW 
            client.println("<h2>Ciepla Woda " + CWSt + "</h2>");
            // If the CWSt is zamknieta, it displays the ON button       
            if (CWSt=="zamknieta") {
              client.println("<p><a href=\"/CW/on\"><button class=\"button\">OTWORZ</button></a></p>");
              client.println("<p><a href=\"/CW/part\"><button class=\"button button3\">UCHYL</button></a></p>");
              client.println("<p> -----------------  </p>");
            } 
            else if(CWSt=="otwarta") {
              client.println("<p><a href=\"/CW/off\"><button class=\"button button2\">ZAMKNIJ</button></a></p>");
              client.println("<p><a href=\"/CW/part\"><button class=\"button button3\">UCHYL</button></a></p>");
              client.println("<p> -----------------  </p>");
            } 
            else if (CWSt=="uchylona"){
              client.println("<p><a href=\"/CW/on\"><button class=\"button\">OTWORZ</button></a></p>");
              client.println("<p><a href=\"/CW/off\"><button class=\"button button2\">ZAMKNIJ</button></a></p>");
              client.println("<p> -----------------  </p>");
              }
           
               
            // Display current state, and ON/OFF buttons for ZW  
            client.println("<h2>Zimna Woda " + ZWSt + "</h2>");
            // If the ZWSt is zamknieta, it displays the otwartaORZ button       
            
            if (ZWSt=="zamknieta") {
              client.println("<p><a href=\"/ZW/on\"><button class=\"button\">OTWORZ</button></a></p>");
              client.println("<p><a href=\"/ZW/part\"><button class=\"button button3\">UCHYL</button></a></p>");
            } 
            else if(ZWSt=="otwarta") {
              client.println("<p><a href=\"/ZW/off\"><button class=\"button button2\">ZAMKNIJ</button></a></p>");
              client.println("<p><a href=\"/ZW/part\"><button class=\"button button3\">UCHYL</button></a></p>");        
            }
            
            else if (ZWSt=="uchylona"){
              client.println("<p><a href=\"/ZW/on\"><button class=\"button\">OTWORZ</button></a></p>");
              client.println("<p><a href=\"/ZW/off\"><button class=\"button button2\">ZAMKNIJ</button></a></p>");
            }
            
            client.println("<p> -----------------  </p>");
            

            // Display current state, and ON/OFF buttons for PRAD);
            // If the PRSt is wyłaczony, it displays the ZALACZ button 
            
            client.println("<h2>Prad L1 " + PRSt1 + "</h2>");          
            if (PRSt1=="wylaczony") {
              client.println("<p><a href=\"/PR1/on\"><button class=\"button\">ZALACZ</button></a></p>");
            } 
            else if(PRSt1=="zalaczony") {
              client.println("<p><a href=\"/PR1/off\"><button class=\"button button2\">WYLACZ</button></a></p>");
            }
         
            client.println("<h2>Prad L2 " + PRSt2 + "</h2>");               
            if (PRSt2=="wylaczony") {
              client.println("<p><a href=\"/PR2/on\"><button class=\"button\">ZALACZ</button></a></p>");
            } 
            else if(PRSt2=="zalaczony") {
              client.println("<p><a href=\"/PR2/off\"><button class=\"button button2\">WYLACZ</button></a></p>");
            }
           
            client.println("<h2>Prad L3 " + PRSt3 + "</h2>");               
            if (PRSt3=="wylaczony") {
              client.println("<p><a href=\"/PR3/on\"><button class=\"button\">ZALACZ</button></a></p>");
            } 
            else if(PRSt3=="zalaczony") {
              client.println("<p><a href=\"/PR3/off\"><button class=\"button button2\">WYLACZ</button></a></p>");
            }
            
            
            client.println("</body></html>");

            
                      
            // The HTTP response ends with another blank line
            client.println();
            
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void Otworz(char woda[2]){
  if (woda == "CW"){
    digitalWrite(CWO, HIGH);
    delay(15000);
    digitalWrite(CWO, LOW);
    }
  if (woda == "ZW"){
    digitalWrite(ZWO, HIGH);
    delay(15000);
    digitalWrite(ZWO, LOW);
    }
  }

void Zamknij(char woda[2]){
  if (woda == "CW"){
    digitalWrite(CWZ, HIGH);
    delay(15000);
    digitalWrite(CWZ, LOW);
    }
  if (woda == "ZW"){
    digitalWrite(ZWZ, HIGH);
    delay(15000);
    digitalWrite(ZWZ, LOW);
    }
  }

void Przymknij(char woda[2]){
  if (woda == "CW"){
    digitalWrite(CWZ, HIGH);
    delay(15000);
    digitalWrite(CWZ, LOW);
    delay(500);
    digitalWrite(CWO, HIGH);
    delay(2050);
    digitalWrite(CWO, LOW);
  }
  else if (woda == "ZW"){
    digitalWrite(ZWZ, HIGH);
    delay(15000);
    digitalWrite(ZWZ, LOW);
    delay(500);
    digitalWrite(ZWO, HIGH);
    delay(2550);
    digitalWrite(ZWO, LOW);
    }
  }

  
