#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include "Tracer.h"

//#include <Wire.h>
#include "myI2c.h"

#define OTA
#define HOST_NAME "esp8266-stepper"
#define HTTP_USERNAME  "admin"
#define HTTP_PASSWORD "admin"

// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

DNSServer dns;

int connectedClients = 0;

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
  if(type == WS_EVT_CONNECT)
  {
    client->ping();
    ws.printfAll("Connected\n");
    if (connectedClients==0)
    {
    }
    connectedClients++;
  }
  else if(type == WS_EVT_DISCONNECT)
  {
    connectedClients--;
    if (connectedClients==0)
    {
    }
  }
  else if(type == WS_EVT_ERROR)
  {
    //Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  }
  else if(type == WS_EVT_PONG)
  {
    //Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  }
  else if(type == WS_EVT_DATA)
  {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len)
    {
        if(info->opcode == WS_BINARY)
        {
            //int16_t av = i2s_available();

            //int16_t written = aoQueue((short *)data, len/2);
            ESP.wdtFeed();
            //ws.printfAll("w: %i, %i, %i", av, i2s_available(), written);
        }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  //Wire.begin(D1,D2);
  //twi_setClock(400000);

  my_twi_setClock(400000);
  my_twi_init(D1,D2);

  // Wait for connection
  AsyncWiFiManager wifiManager(&server,&dns);
  wifiManager.autoConnect("AutoConnectAP");

  //Send OTA events to the browser
  //
  #ifdef OTA
    ArduinoOTA.setHostname(HOST_NAME);
    ArduinoOTA.begin();
    Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", HOST_NAME);
  #endif

  // Start MSDN
  //
  if ( MDNS.begin ( HOST_NAME ) )
  {
      MDNS.addService("http", "tcp", 80);
      Serial.println ( "MDNS responder started" );
  }

  // Start Spiffs
  //
  {
      SPIFFS.begin();
      server.addHandler(new SPIFFSEditor(HTTP_USERNAME, HTTP_PASSWORD));
      server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
      Serial.println ( "SPIFFS started" );
  }

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

////////////////////////////////

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200, "text/plain", "heap: "+String(ESP.getFreeHeap()));
  });

  server.on("/i2cScan", HTTP_GET, [](AsyncWebServerRequest *request)
  {
      String res = "";

      for(int address = 1; address < 127; address++ )
      {
        /*
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();

        if (error == 0)
        {
          res += address;
          res += ", ";
        }
        */
      }

      request->send(200, "text/plain", "i2cScan: " + res + ". Done");
  });
/*
  server.on("/i2c", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    int port = -1;
    int val = 0;

    int params = request->params();
    for(int i=0;i<params;i++)
    {
        AsyncWebParameter* p = request->getParam(i);
        if(p->isFile()){
          Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } else if(p->isPost()){
          Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        } else {
          Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());

          if (p->name()=="port")
            port = p->value().toInt();
          else if (p->name()=="val")
            val = p->value().toInt();
        }
    }

    String message;
    if (port>=0)
    {
      Wire.beginTransmission(port);
      Wire.write(val);
      byte error = Wire.endTransmission();
      message = "port: " + String(port) + "  val: " + String(val) + "  err: " + String(error);
    }

    request->send(200, "text/plain", "Hello, POST: " + message);
  });
*/
/*
  server.on("/gpo", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    int status=-1;
    int pin=-1;

    int params = request->params();
    for(int i=0;i<params;i++)
    {
      AsyncWebParameter* p = request->getParam(i);
      if (p->name()=="status")
        status = p->value().toInt();
      else if (p->name()=="pin")
        pin = p->value().toInt();
    }

    if (pin>=0)
    {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, (status==0)?LOW:HIGH);
    }

    request->send(200, "text/plain", "pin: " + String(pin) + ", status: " + String(status));
  });
*/
  server.on("/sleep", HTTP_GET, [](AsyncWebServerRequest *request)
  {
      request->send(200, "text/plain", "ok");
      ESP.deepSleep(0xffffffffffffffff);
  });

  Tracer_SetHandlers(&server, &ws);

  server.onNotFound([](AsyncWebServerRequest *request)
  {
      String message = "File Not Found\n\n";
      message += "URI: ";
      message += request->url();
      message += "\nMethod: ";
      message += (request->method() == HTTP_GET)?"GET":"POST";
      message += "\nArguments: ";
      message += request->args();
      message += "\n";
      for (uint8_t i=0; i<request->args(); i++){
      message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
      }
      request->send(404, "text/plain", message);
  });

  // Start the webserver
  //
  server.begin();
  Serial.println("Webserver started ");
}

void loop()
{
    MDNS.update();
    ArduinoOTA.handle();
/*
    if (IsTracerRunning())
    {
        StepTrace();
        delayMicroseconds(1000);
    }
*/    
}
