#include <Arduino.h>
#include "MPU6050_6Axis_MotionApps20.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AsyncElegantOTA.h>

void meansensors();
void readCalibration();
void writeCalibration();
void Heartbeat();

ADC_MODE(ADC_VCC);         // to use getVcc

//AP mode is too sluggish so test ran from HooToo hotspot
//#define Wifi_AP_Access
#ifdef Wifi_AP_Access
const char* ssid = "DualLevel";
const char* password = "11111111";
#else
const char* ssid = "BystronicAM";
const char* password = "07880196169";
#endif

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
String strLastMessageSent = "";

MPU6050 mpu;
uint16_t packetSize=0;
uint16_t fifoCount=0;
uint8_t fifoBuffer[42];   // enough for 1 packet of data
Quaternion q;
VectorFloat gravity;
float ypr[3];

float mean_p, mean_r, caly=0, calz=0, temperature=0;

// Initialize WiFi
void initWiFi() {

#ifdef Wifi_AP_Access
    WiFi.mode(WIFI_AP);
    WiFi.begin(ssid, password);
    Serial.println("AP Mode, ip 192.168.4.1");
#else    
// Connect to existing network
  WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(1000);  }
    Serial.println(WiFi.localIP());
#endif
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) { 
    data[len] = 0;
    StaticJsonDocument<100> jsonReceived;
    deserializeJson(jsonReceived,(char*)data);
     // check for calibration request
     if((bool)jsonReceived["Calibrate"])
        {  
          caly = mean_p + caly - 6;  //reset to 6 degrres
          calz = mean_r + calz;      //reset to 0
          writeCalibration();
        }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
 void *arg, uint8_t *data, size_t len) {
 switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      //force server to send current status to browser(s)
      strLastMessageSent="";
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
    break;
  }
}

void sendMessage()
  {
  StaticJsonDocument<100> jsonSend;
  //set up message to send to browser(s)
    //convert axis reading 1 decimal place
    char buffer[10];
    
    // Axis 1 - 6 degrees
    sprintf(buffer,"%.1f",mean_p);
    jsonSend["meanP"] = buffer;    
    
    //axis 2 Level
    sprintf(buffer,"%.1f",mean_r);
    jsonSend["meanR"] = buffer;
    
    //battery voltage
    uint32_t BattVcc = ESP.getVcc();
    float fltBattery = (float)BattVcc / (float)675;
    if(fltBattery>4.3) fltBattery=4.3;
    if(fltBattery<3.3) fltBattery=3.3;
    sprintf(buffer,"%.2f",fltBattery);
    jsonSend["batVoltage"] = buffer;
    
    //temperature
    sprintf(buffer,"%.0f",temperature);
    jsonSend["Temperature"] = buffer;
    
    String strNewMessage = "";
    serializeJson(jsonSend,strNewMessage);
  //only send if the contents have changed from previous message
    if (strNewMessage != strLastMessageSent)
    {
        ws.textAll(strNewMessage);
        strLastMessageSent = strNewMessage;
    }
  }

void setup(){
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,LOW);
 
  Wire.begin();
  Wire.setClock(400000);
    
  initWiFi();
  LittleFS.begin();
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(LittleFS, "/index.html", "text/html",false); });
  server.serveStatic("/", LittleFS, "/");
  AsyncElegantOTA.begin(&server);
  server.begin();

  readCalibration();

  mpu.initialize();
  mpu.setXAccelOffset(1081); 
  mpu.setYAccelOffset(-1246);
  mpu.setZAccelOffset(1326);
  mpu.setXGyroOffset(107);
  mpu.setYGyroOffset(-38);
  mpu.setZGyroOffset(6);
  mpu.resetFIFO();
  mpu.dmpInitialize();
  packetSize = mpu.dmpGetFIFOPacketSize();
  mpu.setDMPEnabled(true); 
}

void loop()
{ 
    meansensors();
    ws.cleanupClients(); 
    AsyncElegantOTA.loop();
    Heartbeat(); 
}
void Heartbeat(){
  static unsigned long blink=0;
  if(blink < millis()){
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    blink += 1500 * digitalRead(LED_BUILTIN) + 25;
  }
}
void meansensors() {
float buff_p=0 ,buff_r=0; 
  //exit until 10 packets are ready
  if(mpu.getFIFOCount() > 419)
  {
    //read packet by packet and make the calculations
    for (int i=0;i<10;i++)
     {
      mpu.getFIFOBytes(fifoBuffer,packetSize);
      mpu.dmpGetQuaternion(&q,fifoBuffer);
      mpu.dmpGetGravity(&gravity,&q);
      mpu.dmpGetYawPitchRoll(ypr,&q,&gravity);                   
      buff_p += ypr[1];
      buff_r += ypr[2];
     }
      mean_p = buff_p/10*180/PI-caly;
      mean_r = buff_r/10*180/PI-calz;
      temperature = mpu.getTemperature() /340+36.53;
      sendMessage(); //update browser
  }
}

void readCalibration()
{
    StaticJsonDocument<100> jsonRead;
    String strRead="";
    File file = LittleFS.open("/calibration", "r");
    if(file)
    {
      strRead = file.readStringUntil('\n');
      file.close();

      Serial.println(strRead);

      deserializeJson(jsonRead,strRead);
      caly = jsonRead["CalibrateY"];
      calz = jsonRead["CalibrateZ"];
    }
}

void writeCalibration()
{   
    StaticJsonDocument<100> jsonWrite;
    String strWrite="";
    jsonWrite["CalibrateY"] = caly;
    jsonWrite["CalibrateZ"] = calz;
    serializeJson(jsonWrite,strWrite);
    File file = LittleFS.open("/calibration", "w");
    if (file)
    {
      file.println(strWrite);
      file.close();

      Serial.println(strWrite);

    }
}