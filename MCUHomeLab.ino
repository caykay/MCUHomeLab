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
  server->onNotFound([server]{
    server->send(404, "text/html", 
          "<!DOCTYPE html>\
            <html>\
              <body>\
                <p>Page not found </p>\
              </body>\
            </html>\
          "
          );
  });
}

void handleConfigSetupRequest()
{
  if (APServer == nullptr)
    return;

   APServer->send(200, "text/html", 
          "<!DOCTYPE html>\
            <html>\
              <body>\
                <p>Configure the Home Lab wifi:</p>\
                <form method='post' action='/connect-wifi'>\
                  <input type='text' placeholder='ssid' required/>\
                  <input type='password' placeholder='password' required/>\
                  <input type=\"submit\" value=\"Connect\"/>\
                </form>\
              </body>\
            </html>\
          "
          );
}

void handleWifiConnection()
{
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
    APServer->on("/", [&APServer]{
      APServer->send(200, "text/html", 
        "<!DOCTYPE html>\
          <html>\
            <body>\
              <p>Welcome to my ESP32 Board</p>\
              <form action=\"/wifi-setup\">\
                <input type=\"submit\" value=\"Configure Wifi\"/>\
              </form>\
            </body>\
          </html>\
        "
      );
    });

    // handle not found
    handleNotFound(APServer);

    APServer->on("/wifi-setup", handleConfigSetupRequest);

    // TODO: handle config settings here
    APServer->on(
      "/connect-wifi", HTTP_POST,
      []{
        Serial.println("Yayyy you've gotten somewhere");
      }
    );

    Serial.print("Started APServer. Now listening at: ");
    Serial.print(WiFi.softAPIP());
    Serial.print(":");
    Serial.println(Port);
  }
}

void loop()
{
  if (APServer != nullptr)
    APServer->handleClient();

  delay(2);

  if (PublicServer != nullptr)
    PublicServer->handleClient();
  delay(2);
}
