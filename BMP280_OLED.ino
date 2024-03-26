/*
 * W bibliotece - /home/marcin/sketchbook/
 * libraries/Adafruit_BMP280_Library/Adafruit_BMP280.cpp
 * w linii 71 zmiana "addr" na rzeczywysty adres I2C sensora.
 * 
 * normalnie 0x77 (zostawić addr) ,zmieniony- 0x76
 * 
 * A4 - SDA, A5-SCL
 * 
 * bmp.readAltitude(1021) - miejscowe ciśnienie zredukowane do 
 * poziomu morza
 * 
 * This code is to use with Adafruit BMP280 and OLED screen   (Metric)
 * It measures both temperature and pressure and it displays them on the 
 * OLED display with the altitude
 * It's a modified version of the Adafruit example code
 * Refer to www.surtrtech.com or SurtrTech Youtube channel
 */

#include <Adafruit_GFX.h>      //Libraries for the OLED and BMP280
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
float actPress = 1021;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Declaring the display name (display)
Adafruit_BMP280 bmp;

void setup() {  
  Serial.begin(115200);
  bmp.begin();                                //Start the bmp                  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Start the OLED display
  display.clearDisplay();
  display.display();
  display.setTextColor(WHITE);
  display.setTextSize(1); 
  display.print("M.Mierzejewski 2019");     //Show the name, you can remove it or replace it
  display.setCursor(0,32);
  display.setTextSize(1);          
  display.println("BMP280"); 
  display.display();
    
  delay(1000);
}

void loop() {

    display.clearDisplay();
    float T = bmp.readTemperature();          //Read temperature in C
    float P = bmp.readPressure()/100;         //Read Pressure in Pa and conversion to hPa
    float A = bmp.readAltitude(actPress);     //Calculating the Altitude, the "1019.66" is the pressure in (hPa) at sea level at day in your region
                                              
    
    display.setCursor(0,24);                   //Oled display, 
    display.setTextSize(2); 
    display.print("Tmp");
    
    display.setCursor(48,24);
    display.print(T,1);
    display.setCursor(110,24);
    display.setTextSize(1);
    display.print("C");

    display.setTextSize(2);
    display.setCursor(0,0);
    display.print(P);
    display.setTextSize(1);
    display.setCursor(90,8);
    display.print("hPa");

    display.setTextSize(2);
    display.setCursor(0,48);
    display.print("Wys");
    display.setCursor(48,48);
    display.print(A,0);
    display.setCursor(100,48);
    display.print("m");
    
    display.display();

    Serial.print(F("Temperature = "));
    Serial.print(T);
    Serial.println(" *C");
  
    Serial.print(F("Pressure = "));
    Serial.print(P); 
    Serial.println(" hPa");
  
    Serial.print(F("Approx altitude = "));
    Serial.print(A);      
    Serial.println(" m"); 
    Serial.println();
    delay(2000);
}
