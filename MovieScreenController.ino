#define DEBUG

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <M5Stack.h> 

#include "secrets.h"
#include "Signals.h"

const int DOWN=3;   // gray
const int STOP=16;  // black
const int UP=2;     // white

const int SIGNAL_TIME_MS=1000;

const int SCREEN_MOVE_TIME_MS=45200;

const int LCD_ON_TIME_MS=10000;

WebServer server(80);

const int led = 13;

int lcdOnTime=0;
int screenMode=0;

const signal DOWN_SIGNAL            = 0;
const signal STOP_SIGNAL            = 1;
const signal UP_SIGNAL              = 2;
const signal HEARTBEAT_SIGNAL       = 3;
const signal INIT_SIGNAL            = 4;
const signal STATE_SIGNAL           = 5;
const signal NETWORK_HANDLER_SIGNAL = 6;


Signals signals([](signal s) { 
  MyDebug::printDebug("Clear screen for signal!");
  M5.Lcd.clear(); 
  });

int oldBatteryLevel = 0;
bool oldIsCharging = false;

const int batteryLevels[4]={25,50,75,100};
const int batterySegmentWidth = 73; // pixel
const int segmentGap = 2;

bool sleepingLcd = false;

void SleepLcd() {
  if ( !sleepingLcd ) { 
    M5.Lcd.sleep();
    M5.Lcd.clear();
    sleepingLcd = true;
  }
}

void WakeUpLcd() {
  if ( sleepingLcd ) {
    M5.Lcd.wakeup();
    M5.Lcd.clear();   
    sleepingLcd = false;
  }
}

void drawBatteryState(int batteryLevel, boolean isCharging) {
  int levelColor = TFT_BLUE;
 
  for ( int i = 0; i < 4; i++ ) {
    if ( batteryLevel < batteryLevels[i] ) levelColor = TFT_BLACK;
    M5.Lcd.fillRect(1+(batterySegmentWidth+segmentGap)*i, 5, batterySegmentWidth, 10, levelColor); 
  }

  M5.Lcd.fillCircle(310,10,4, isCharging ? TFT_YELLOW : TFT_BLACK);    
}

void HandleDisplay() {
  if (signals.noneTriggered() ) {
    SleepLcd();
    return;
  } else {
    WakeUpLcd();  
  }

  int batteryLevel = M5.Power.getBatteryLevel();
  bool isCharging = M5.Power.isCharging();

  if ( signals.isTriggered(INIT_SIGNAL) ) {
    M5.Lcd.setBrightness(100);    
    drawBatteryState(batteryLevel, isCharging);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setCursor(10,20);
    M5.Lcd.print("IP: ");
    M5.Lcd.println(WiFi.localIP());     
  } else if ( signals.isTriggered(DOWN_SIGNAL) ) {
    M5.Lcd.setBrightness(100);    
    drawBatteryState(batteryLevel, isCharging);
    M5.Lcd.fillRect(150, 110, 20, 60, TFT_GREEN);
    M5.Lcd.fillTriangle(145, 170, 175, 170, 160, 190, TFT_GREEN);
  } else if ( signals.isTriggered(UP_SIGNAL) ) {
    M5.Lcd.setBrightness(100);    
    drawBatteryState(batteryLevel, isCharging);
    M5.Lcd.fillTriangle(145, 130, 175, 130, 160, 110, TFT_GREEN);
    M5.Lcd.fillRect(150, 130, 20, 60, TFT_GREEN);
  } else if ( signals.isTriggered(STOP_SIGNAL) ) {
    M5.Lcd.setBrightness(100);    
    drawBatteryState(batteryLevel, isCharging);
    M5.Lcd.fillTriangle(110, 140, 140, 140, 140, 110, TFT_RED);
    M5.Lcd.fillTriangle(160, 110, 160, 140, 190, 140, TFT_RED);
    M5.Lcd.fillRect(140, 110, 20, 80, TFT_RED);
    M5.Lcd.fillRect(110, 140, 80, 20, TFT_RED);   
    M5.Lcd.fillTriangle(110, 160, 140, 160, 140, 190, TFT_RED);
    M5.Lcd.fillTriangle(160, 160, 190, 160, 160, 190, TFT_RED);
  } else if ( signals.isTriggered(STATE_SIGNAL ) ) {
    M5.Lcd.setBrightness(100);    
    drawBatteryState(batteryLevel, isCharging);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setCursor(10,20);
    M5.Lcd.print("IP: ");
    M5.Lcd.println(WiFi.localIP());      
    if ( screenMode == DOWN ) {
      M5.Lcd.fillRect(10,50,300,180, TFT_WHITE);
    } else {
      M5.Lcd.fillRect(10,50,300,2, TFT_WHITE);
    }
  } 
}

// =-------------------------
// =-------------------------

void handleRoot() {
  digitalWrite(led, 1);
  char temp[800];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 800,

           "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP32 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Screen controller</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <a href='/movie/on'>[/movie/on] - screen down</a><br>\
    <a href='/movie/off'>[/movie/off] - screen up</a><br>\
    <a href='/movie/stop'>[/movie/stop] - stop screen</a><br>\
    <a href='/movie/state'>[/movie/state] - state</a>\
  </body>\
</html>",

           hr, min % 60, sec % 60
          );
  server.send(200, "text/html", temp);
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void screenStop() {
  MyDebug::printDebug("Stop the screen...");
  digitalWrite(STOP, HIGH);
  delay(SIGNAL_TIME_MS);
  digitalWrite(STOP,LOW);  
  
  signals.clear(UP_SIGNAL);
  signals.clear(DOWN_SIGNAL);
  signals.fire(STOP_SIGNAL);  
}

void screenDownForMovie() {
   MyDebug::printDebug("Scroll down the screen...");
   
   screenMode = DOWN;
   digitalWrite(DOWN,HIGH);
   delay(SIGNAL_TIME_MS);
   digitalWrite(DOWN,LOW);

   signals.fire(DOWN_SIGNAL, SCREEN_MOVE_TIME_MS, [](){ M5.Lcd.fillRect(10,110,300,120, TFT_BLACK); screenStop(); });
}

void screenUpForMovie() {
   MyDebug::printDebug("Scroll up the screen...");
   
   screenMode=UP;
   digitalWrite(UP,HIGH);
   delay(SIGNAL_TIME_MS);
   digitalWrite(UP,LOW);
  
   signals.fire(UP_SIGNAL, SCREEN_MOVE_TIME_MS+1000); // add a bit more time when closing the screen, to be sure it's fully closed (deffensive stuff)   
}

void reportScreenState() {
  MyDebug::printDebug("State request received...");
  
  int batteryLevel = M5.Power.getBatteryLevel();
  bool isCharging = M5.Power.isCharging();
    
  char strStatus[300]; sprintf(strStatus, "{\"batteryLevel\":%d,\"isCharging\": %d, \"state\":\"%s\", \"moveTime\":%d}", batteryLevel, isCharging, (screenMode == DOWN ? "dowm":"up"), SCREEN_MOVE_TIME_MS);

  MyDebug::printDebug(strStatus);
   
  signals.fire(
      HEARTBEAT_SIGNAL, 
      1000, 
      [](milliseconds timeout, milliseconds progressTime) {
        if ( signals.anyTriggeredExcept(HEARTBEAT_SIGNAL) ) {
          MyDebug::printDebug("Skip hartbeat UI");
          return;  
        }
        const float halfTime = (timeout/2.0);
        const float brightnessChangeUnit = 255.0 / halfTime;
        const int brightness = (int)std::floor(brightnessChangeUnit*(-1*std::abs(progressTime - halfTime) + halfTime) +0.5);
             
        M5.Lcd.setBrightness( brightness );
        // M5.Lcd.fillEllipse(160,120,90,90, isCharging ? TFT_YELLOW : (batteryLevel >= 75 ? TFT_GREEN : (batteryLevel <= 25 ? TFT_RED : TFT_ORANGE)));
        M5.Lcd.fillEllipse(160,120,90,90, TFT_ORANGE);
      }, 
      []() { 
        if ( signals.anyTriggeredExcept(HEARTBEAT_SIGNAL) ) {
          MyDebug::printDebug("Skip hartbeat UI clear");
          return;  
        }
        M5.Lcd.clear(); 
      }); 
}

// =-------------------------
// =-------------------------

milliseconds btnBPressed = 0;
void HandleM5Buttons() {
  M5.update();
    
  if ( M5.BtnA.pressedFor(300) ) {
    MyDebug::printDebug("Button-A pressed");
    screenDownForMovie();
  }
  if ( M5.BtnB.pressedFor(200) ) {
    if ( btnBPressed == 0 ) btnBPressed = millis();
  }
  
  if ( M5.BtnC.pressedFor(300) ) {
    MyDebug::printDebug("Button-C pressed");
    screenUpForMovie();
  }

  if ( !M5.BtnB.isPressed() ) {
    if ( btnBPressed != 0 ) {
      if ( millis() - btnBPressed > 1000 ) {
        MyDebug::printDebug("Button-B 2nd function!");
        signals.fire(STATE_SIGNAL, 10000);
      } else if ( millis() - btnBPressed > 300 ) {
        MyDebug::printDebug("Button-B 1st function!");
        screenStop();
      } else  {
        MyDebug::printDebug("Button-B flickering!");
      }
    
      btnBPressed = 0;
    }
  }
  
}

// =-------------------------
// =-------------------------
// =-------------------------
// =-------------------------


void setup(void) { 
  M5.begin();
  M5.Power.begin();

  M5.Lcd.setBrightness(100);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(0x51d);
      
  
  Serial.begin(115200);
  Serial.println("");  

  pinMode(DOWN, OUTPUT);
  pinMode(UP, OUTPUT);
  pinMode(STOP, OUTPUT);
  digitalWrite(DOWN, LOW);
  digitalWrite(UP, LOW);
  digitalWrite(STOP, LOW);  
    
  signals.fire(NETWORK_HANDLER_SIGNAL, 0, [](milliseconds _, milliseconds progressTime) {
    if (WiFi.status() != WL_CONNECTED) {
      MyDebug::printDebug("No WiFi connection!");
      
      WakeUpLcd();
      M5.Lcd.setTextColor(TFT_BLACK);
      M5.Lcd.setCursor(1,10);      
      M5.Lcd.println("Waiting for Wi-Fi connection");
      //  WiFi.mode(WIFI_STA);      
      WiFi.begin(ssid, password);

      int count = 0;
      while ( count < 120 ) {
        if (WiFi.status() == WL_CONNECTED) {
          MyDebug::printDebug("Connected!");
          M5.Lcd.println("Wi-Fi connected!");
          // setup
          char str[100]; 
          IPAddress ip = WiFi.localIP();
          sprintf(str, "Connected to %s\nIP address: %s", ssid, ip.toString() );
          MyDebug::printDebug(str);
        
          if (MDNS.begin("esp32")) {
            MyDebug::printDebug("MDNS responder started");
          }
        
          server.on("/movie/on", []() { screenDownForMovie(); server.send(200,"text/html","done"); });
          server.on("/movie/state", []() { reportScreenState(); server.send(200,"text/html","done"); });
          server.on("/movie/off", []() { screenUpForMovie(); server.send(200,"text/html","done"); });
          server.on("/movie/stop", []() { screenStop(); server.send(200,"text/html","done"); });
          
          server.on("/", handleRoot);
          server.on("/inline", []() {
            server.send(200, "text/plain", "this works as well");
          });
          server.onNotFound(handleNotFound);
          server.begin();   
          MyDebug::printDebug("HTTP server started");
          return;            
        }
        delay(1000);
        Serial.print(".");
        count++;
      }         
    }
  });
}

void loop(void) { 
  signals.processSignals();

  server.handleClient();
    
  HandleM5Buttons();
  
  HandleDisplay();
}
