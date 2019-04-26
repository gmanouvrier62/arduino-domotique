    /////////////////////////////////////////////////////////////////
   //         ESP32 Web Server Project  v1.00                     //
  //       Get the latest version of the code here:              //
 //         http://educ8s.tv/esp32-web-server                   //
/////////////////////////////////////////////////////////////////


#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>  //https://github.com/bbx10/WebServer_tng
#include <OneWire.h>

//include "Adafruit_BME280.h" //https://github.com/Takatsuki0204/BME280-I2C-ESP32

WebServer server ( 80 );

const char* ssid     = "Bbox-AABC88F4";
const char* password = "5E4214AE424CA33E3574E1212ACE6C";
#define ALTITUDE 216.0 // Altitude in Sparta, Greece

#define I2C_SDA 27
#define I2C_SCL 26
#define BME280_ADDRESS 0x76  //If the sensor does not work, try the 0x77 address as well
int LEDPIN = 23;

float temperature = 0;
float humidity = 0;
float pressure = 0;

String  ledState = "OFF";

//Adafruit_BME280 bme(I2C_SDA, I2C_SCL);
float Temperature() {
  
      boolean debug = false;
      byte i;
      byte present = 0;
      byte type_s;
      byte data[12];
      byte addr[8];
      float celsius, fahrenheit;
      
      if ( !ds.search(addr)) {
        /*
        if(debug)
        {
        Serial.println("No more addresses.");
        Serial.println();
        }
        */
        ds.reset_search();
        delay(250);
        return 0;
      }
      
      /*
      if(debug)
      {
      
        Serial.print("ROM =");
        for( i = 0; i < 8; i++) {
          Serial.write(' ');
          Serial.print(addr[i], HEX);
        }
    
      }
      */
      if (OneWire::crc8(addr, 7) != addr[7]) {
          Serial.println("CRC is not valid!");
          return 0;
      }
      Serial.println();
     
      // the first ROM byte indicates which chip
      if(debug)
      {
      
          switch (addr[0]) {
            case 0x10:
              Serial.println("  Chip = DS18S20");  // or old DS1820
              type_s = 1;
              break;
            case 0x28:
              Serial.println("  Chip = DS18B20");
              type_s = 0;
              break;
            case 0x22:
              Serial.println("  Chip = DS1822");
              type_s = 0;
              break;
            default:
              Serial.println("Device is not a DS18x20 family device.");
              return 0;
          } 
      }
      ds.reset();
      ds.select(addr);
      ds.write(0x44, 1);        // start conversion, with parasite power on at the end
      
      delay(1000);     // maybe 750ms is enough, maybe not
      // we might do a ds.depower() here, but the reset will take care of it.
      
      present = ds.reset();
      ds.select(addr);    
      ds.write(0xBE);         // Read Scratchpad
    
      if(debug)
      {
        Serial.print("  Data = ");
        Serial.print(present, HEX);
        Serial.print(" ");
      }
      for ( i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = ds.read();
        if(debug)
        {
          Serial.print(data[i], HEX);
          Serial.print(" ");
        }
      }
      if(debug)
      {
        Serial.print(" CRC=");
        Serial.print(OneWire::crc8(data, 8), HEX);
        Serial.println();
      }
      // Convert the data to actual temperature
      // because the result is a 16 bit signed integer, it should
      // be stored to an "int16_t" type, which is always 16 bits
      // even when compiled on a 32 bit processor.
      int16_t raw = (data[1] << 8) | data[0];
      if (type_s) {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
          // "count remain" gives full 12 bit resolution
          raw = (raw & 0xFFF0) + 12 - data[6];
        }
      } else {
        byte cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time
      }
      celsius = (float)raw / 16.0;
      celsius=celsius-2;
      fahrenheit = celsius * 1.8 + 32.0;
     if(debug)
     {
      Serial.print("  Temp = ");
      Serial.print(celsius);
     }
     else
     {
      Serial.print("D");
      Serial.print(celsius);
      Serial.print("F"); 
      //Serial.print(bcl);
      return celsius;
       
       
     }
      

}
void setup() 
{
  pinMode(LEDPIN, OUTPUT);
  
  Serial.begin(9600);

  //initSensor();

  connectToWifi();

  beginServer();
}

void loop() {
 
 server.handleClient();
 
 getTemperature();
 //getHumidity();
 //getPressure();
 delay(1000);
 
}

void connectToWifi()
{
  WiFi.enableSTA(true);
  
  delay(2000);

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void beginServer()
{
  server.on ( "/", handleRoot );
  server.begin();
  Serial.println ( "HTTP server started" );
}

void handleRoot(){ 
  if ( server.hasArg("LED") ) {
    handleSubmit();
  } else {
    server.send ( 200, "text/html", getPage() );
  }  
}



void handleSubmit() {

  String LEDValue;
  LEDValue = server.arg("LED");
  Serial.println("Set GPIO "); 
  Serial.print(LEDValue);
  
  if ( LEDValue == "1" ) {
    digitalWrite(LEDPIN, HIGH);
    ledState = "On";
    server.send ( 200, "text/html", getPage() );
  }
  else if( LEDValue == "0" ) 
  {
    digitalWrite(LEDPIN, LOW);
    ledState = "Off";
    server.send ( 200, "text/html", getPage() );
  } else 
  {
    Serial.println("Error Led Value");
  }
}

String getPage(){
  String page = "<html lang=en-EN><head><meta http-equiv='refresh' content='60'/>";
  page += "<title>ESP32 WebServer - educ8s.tv</title>";
  page += "<style> body { background-color: #fffff; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }</style>";
  page += "</head><body><h1>ESP32 WebServer</h1>";
  page += "<h3>BME280 Sensor</h3>";
  page += "<ul><li>Temperature: ";
  page += temperature;
  page += "&deg;C</li>";
  page += "<li>Humidity: ";
  page += humidity;
  page += "%</li>";
  page += "<li>Barometric Pressure: ";
  page += pressure;
  page += " hPa</li></ul>";
  page += "<h3>GPIO</h3>";
  page += "<form action='/' method='POST'>";
  page += "<ul><li>LED";
  page += "";
  page += "<INPUT type='radio' name='LED' value='1'>ON";
  page += "<INPUT type='radio' name='LED' value='0'>OFF</li></ul>";
  page += "<INPUT type='submit' value='Submit'>";

  page += "</body></html>";
  return page;
}

