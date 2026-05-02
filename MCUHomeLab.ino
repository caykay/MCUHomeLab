#include <WiFi.h>
#include <WebServer.h>

#include <memory>

// MCU board constants

constexpr int NPNBasePin = 2;

constexpr int Port = 80;

// WiFi

WebServer* PublicServer;

// AP

WebServer* APServer;
constexpr const char* APSsid = "ESP32 Wifi";
constexpr const char* APPassword = "12345678";

// Config

const char* WifiSsid = "******";
const char* WifiPassword = "******";


// middleware

void handleNotFound(WebServer* server)
{
  server->onNotFound([&server]{
    server->send(404, "text/html", 
          "<!DOCTYPE html>\
            <html>\
              <body>\
                <p>Welcome to my ESP32 Board</p>\
              </body>\
            </html>\
          "
          );
  });
}

void setup()
{
  Serial.begin(115200);

  // setup AP + AP Server
  if (WiFi.softAP(APSsid, APPassword))
  {
    Serial.print("Started WiFi access point with SSID: ");
    Serial.println(APSsid);

    APServer = new WebServer(WiFi.softAPIP(), Port);
    APServer->begin();

    // handle root
    APServer->on("/", []{
      APServer->send(200, "text/html", 
        "<!DOCTYPE html>\
          <html>\
            <body>\
              <p>Welcome to my ESP32 Board</p>\
            </body>\
          </html>\
        "
      );
    });

    // handle not found
    handleNotFound(APServer);

    // TODO: handle config settings here


    Serial.print("Started APServer. Now listening at: ");
    Serial.print(WiFi.softAPIP());
    Serial.print(":");
    Serial.println(Port);
  }
}

void loop()
{
  if (APServer)
    APServer->handleClient();

  if (PublicServer)
    PublicServer->handleClient();
}
