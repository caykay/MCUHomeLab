#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <StreamString.h>

#include "staticcontent.h"

// MCU board constants

constexpr int NPNBasePin = 2;

// WiFi

WebServer STAServer(80);

// AP

DNSServer dnsServer;
WebServer APServer(80);
constexpr const char* APSsid = "ESP32 Wifi";
constexpr const char* APPassword = "12345678";

// Wifi Events

void WiFiEvent(WiFiEvent_t event) 
{
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      // delay a lil bit before switching over from AP to STA
      delay(2000);
      APServer.stop();
      dnsServer.stop();
      WiFi.softAPdisconnect();
      delay(50);
      // start STA server
      WiFi.mode(WIFI_STA);
      STAServer.begin();
      Serial.printf("Started STA server at address: %s\n", WiFi.localIP().toString());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      // restart DNS and AP servers
      // or maybe re-attempt to connect to wifi
      break;
    default: break;
  }
}

// request handlers

void handleNotFound(WebServer& server)
{
  server.onNotFound([&server]{
    server.send(404, "text/html", NotFoundContent);
  });
}


void handleWifiConnection()
{
  String errorMsg = "";
  if (APServer.hasArg("ssid") && APServer.hasArg("password"))
  {
    bool connected = connectWifi(APServer.arg("ssid"), APServer.arg("password"));
    
    if (connected)
    {
      APServer.sendHeader("Location", "/wifi-connected");
      APServer.send(302, "text/plain", "AP and DNS closing soon");
      return;
    }
    else
    {
      Serial.println("Could not connect to the wifi");
      errorMsg = "Failed to connect to the wifi. Try again";
    }
  }

  StreamString content;
  content.printf(APWifiSetupContent, errorMsg);

  APServer.send(200, "text/html", (String)content);
}

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.onEvent(WiFiEvent);

  // WiFi.AP.enableDhcpCaptivePortal();

  // setup AP + AP Server
  setupAPServer();

  setupSTAServer();

  // dns server redirecting all requests from the AP it's ip address
  // which is technically also the APServer's forcing the captive portal
  if (dnsServer.start(53, "*", WiFi.softAPIP()))
  {
    Serial.println("Started DNS server in captive portal-mode");
  }
  else
  {
    Serial.println("Err: Can't start DNS server!");
  }

  APServer.begin();
  Serial.printf("Started APServer at address: %s\n", WiFi.softAPIP().toString());
}

void loop()
{
  dnsServer.processNextRequest();
  APServer.handleClient();
  STAServer.handleClient();

  delay(2);
}

// helper methods

void setupAPServer()
{
  if (WiFi.softAP(APSsid, APPassword))
  {
    // handle root
    APServer.on("/", [&APServer]{
      APServer.send(200, "text/html", APIndexContent);
    });

    // handle not found
    APServer.onNotFound([&APServer]{
      APServer.sendHeader("Location", "/");
      APServer.send(302, "text/plain", "Redirect to captive portal");
    });

    APServer.on("/connect-wifi", handleWifiConnection);

    APServer.on("/wifi-connected", []{
      APServer.send(200, "text/html", APEndedContent);
    });
  }
}

void setupSTAServer()
{
  // handle root
  STAServer.on("/", [&STAServer]{
    STAServer.send(200, "text/html", STAIndexContent);
  });
  // handle not found
  handleNotFound(STAServer);
}

bool connectWifi(String ssid, String password)
{
  WiFi.disconnect(true, true); 

  WiFi.mode(WIFI_AP_STA); // allows the esp32 to act both as a AP and STA
  auto status = WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (status != WL_CONNECTED && millis() - start <= 10'000)
  {
    delay(500);
    status = WiFi.status();
    Serial.print('.');
  }

  if (millis() != start)
    Serial.println();
  return status == WL_CONNECTED;
}
