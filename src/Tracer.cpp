#include "vector"
#include <ESPAsyncWebServer.h>
#include "Bresenham.h"
#include "AudioIn.h"
#include "printerDriver.h"

struct MoveTo
{
  uint16_t x,y;
};

static std::vector<MoveTo> lineList;

static uint16_t iIndex = 0;

static Bresenham bh;

static unsigned long _time, _steps;

static AsyncWebSocket *pWs;

static void ICACHE_RAM_ATTR Step();
static void ICACHE_RAM_ATTR Home();

static char operationInProgress[] = "Operation in progress";

void Tracer_SetHandlers(AsyncWebServer *pServer, AsyncWebSocket *pWebSocket)
{
  pWs = pWebSocket;
  pServer->on("/MoveTo", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    MoveTo mt;
    mt.x = UINT16_MAX;
    mt.y = UINT16_MAX;

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

          if (p->name()=="x")
            mt.x = p->value().toInt();
          else if (p->name()=="y")
            mt.y = p->value().toInt();
        }
    }

    if (mt.x != INT_MAX && mt.y!=INT_MAX)
    {
      lineList.push_back(mt);
      request->send(200, "text/plain", "OK");
    }
    else
    {
      String message  = "MoveTo:   p0: (" + String(mt.x) + ", " + String(mt.y) + ") ";
      request->send(200, "text/plain", message);
    }
  });

  pServer->on("/Home", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (aiIsRunning())
    {
      request->send(200, "text/plain", operationInProgress);
      return;
    }

    int speed = 1000;
    aiBegin(Home, speed);

    request->send(200, "text/plain", "Homing...");
  });

  pServer->on("/Start", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (aiIsRunning())
    {
      request->send(200, "text/plain", operationInProgress);
      return;
    }

    iIndex = 0;
    _steps = 0;
    int speed = 1000;

    if (lineList.size()>0)
    {
      AsyncWebParameter* p = request->getParam(0);
      if (p!=NULL && p->name()=="speed")
        speed = p->value().toInt();

      request->send(200, "text/plain", "Starting...");
      _time = millis();
      aiBegin(Step, speed);
      return;
    }

    request->send(200, "text/plain", "No code to process");
  });

  pServer->on("/Post", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total)
  {
    if (aiIsRunning())
    {
      request->send(200, "text/plain", operationInProgress);
      return;
    }

    pWs->printfAll("post %u, %u, %u\n", len,index, total);

    if (index==0)
    {
      lineList.resize(total/2/2);
    }

    memcpy(((uint8_t *)lineList.data()) + index, data, len);

    if (index+len==total)
      request->send(200, "text/plain", "OK");
  });

  pServer->on("/ReadSwitches", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (aiIsRunning())
    {
      request->send(200, "text/plain", operationInProgress);
      return;
    }

    //read limiting switches
    bool switchLimitX;
    bool switchLimitY;
    bool switchLimitZ;
    ReadSwitches( &switchLimitX, &switchLimitY, &switchLimitZ);

    String message = "{ \"X\":" + String(switchLimitX) + ", \"Y\":" + String(switchLimitY) + ", \"Z\":" + String(switchLimitZ) + " }";

    request->send(200, "application/json", message);
  });

  pServer->on("/Status", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String message  = "Status:\n iIndex: " + String(iIndex) + "\n size: " + String(lineList.size()) + "\n";
    for (uint16_t i=0;i<lineList.size();i++)
    {
      message  += "mt: (" + String(lineList[i].x) + ", " + String(lineList[i].y) + ")\n";
    }


    message  += "running: ";
    if (aiIsRunning())
      message  += "true";
    else
      message  += "false";

    message  += "\n";

    message  += "Done.\n";

    request->send(200, "text/plain", message);
  });

  pServer->on("/clear", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (aiIsRunning())
    {
      request->send(200, "text/plain", operationInProgress);
      return;
    }

    lineList.clear();
    String message  = "Clear:\n iIndex: " + String(iIndex) + "\n size: " + String(lineList.size());
    request->send(200, "text/plain", message);
  });

}

void ICACHE_RAM_ATTR Home()
{
  bool bDone = HomeRobot();
  if (bDone)
  {
    aiEnd();
    DisableMotors();
    pWs->printfAll("Homing done\n");
  }
}

void ICACHE_RAM_ATTR Step()
{
   static bool bDone = true;

   if (bDone == true)
   {
      MoveTo ini = lineList[iIndex++];

      if (iIndex == lineList.size())
      {
        aiEnd();
        DisableMotors();

        _time = millis() - _time;
        pWs->printfAll("Done, steps: %lu, time: %lu ms, freq: %lu steps/sec\n", _steps, _time, _steps*1000 / _time);

        iIndex = 0;

        return;
      }

      MoveTo fin = lineList[iIndex];
      bDone=bh.Init(ini.x,ini.y,fin.x,fin.y);

      //pWs->printfAll("--mt %i, %i\n", fin.x,fin.y);
   }

  if (bDone == false)
  {
      // compute deltas
      int stepX, stepY;
      bDone = bh.Tick(&stepX, &stepY);
/*
      int x,y;
      bh.GetPos(&x,&y);
      pWs->printfAll("-- %i, %i   (%i, %i)\n",x,y, stepX, stepY);
*/
      MoveRobot(stepX, stepY, 0);
      _steps++;
  }
}
