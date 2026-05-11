#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Ticker.h>
#include <StreamString.h>
// this is not ideal (i.e. lose benefit of code autocompletion) -> to be switched with serving html content from the file system
// this can be once we have the micro SD modules setup. i.e. could also maybe have macros that check if microSD is setup otherwise use the header content
// which will be must simpler to preserve RAM uses
#include "staticcontent.h"

// MCU board

constexpr int LEDPin = 2;
constexpr int RESETPin = 5;
Preferences Preferences;
Ticker resetSwitchTicker;

// WiFi

WebServer STAServer(80);
static bool STAConnected = false;
constexpr const char* Hostname = "myesp32";

constexpr int32_t WIFI_SSID_LIMIT_MAX = 32;
constexpr int32_t WIFI_PASS_LIMIT_MIN = 8;
constexpr int32_t WIFI_PASS_LIMIT_MAX = 64;

// AP

DNSServer dnsServer;
WebServer APServer(80);
constexpr const char* APSsid = "ESP32 Wifi";
constexpr const char* APPassword = "12345678";

// Wifi Events

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      // delay a lil bit before switching over from AP to STA
      delay(2000);
      APServer.stop();
      dnsServer.stop();
      WiFi.softAPdisconnect();
      delay(50);
      // start mdn server? allows use to use hostname.local to query the STA web server
      startMDNS();
      // start STA server
      WiFi.mode(WIFI_STA);
      STAServer.begin();
      Serial.printf("Started STA server at address: %s\n", WiFi.localIP().toString());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      if (STAConnected)
      {
        Serial.printf("WiFi connection lost, reason: %s\n", info.wifi_sta_disconnected.reason);
        WiFi.reconnect();
      }
      break;
    default: break;
  }
}

// request handlers

void handleGetLEDState()
{
  bool status = digitalRead(LEDPin);
  StreamString json;
  json.printf(R"(["state": "%s"])", !status ? "on" : "off");
  STAServer.send(200, "application/json", (String)json);
}

void handleToggleLED()
{
  bool status = digitalRead(LEDPin);
  digitalWrite(LEDPin, !status);
  handleGetLEDState();
}

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
  pinMode(LEDPin, OUTPUT);
  pinMode(RESETPin, INPUT_PULLUP);
  Serial.begin(115200);

  WiFi.onEvent(WiFiEvent);

  setupAPServer();
  setupSTAServer();

  // attempt to read wifi credentials from EEPROM otherwise launch captive portal
  String ssid = String(), password = String();
  if(readWifiCredentials(ssid, password))
  {
    Serial.printf("Found saved wifi credentials for ssid: %s\n", ssid);
    WiFi.mode(WIFI_STA);
    connectWifi(ssid, password);
    return;
  }

  WiFi.mode(WIFI_AP_STA); // allows the esp32 to act both as a AP and STA

  // dns server redirecting all requests from the AP it's ip address
  // which is technically also the APServer's forcing the captive portal
  if (dnsServer.start(53, "*", WiFi.softAPIP()))
    Serial.println("Started DNS server in captive portal-mode");
  else
    Serial.println("Err: Can't start DNS server!");

  APServer.begin();
  Serial.printf("Started APServer at address: %s\n", WiFi.softAPIP().toString());
}

void loop()
{
  dnsServer.processNextRequest();
  APServer.handleClient();
  STAServer.handleClient();

  // board btn handles
  handleEEPROMResetBtn(); // listen for reset btn being pressed (reset eeprom and restart esp after 2 seconds of helding btn)

  delay(2);
}

// helper methods

// write wifi credentials to EEPROM
bool saveWiFiCredentials(const String& ssid, const String& password)
{
  bool success = true;
  success &= Preferences.begin("credentials", false);
  success &= Preferences.putString("ssid", ssid);
  success &= Preferences.putString("pass", password);
  Preferences.end();
  if (!success)
    Serial.printf("Failed to save wifi credentials for ssid: %s", ssid);
  return success;
}

// Read credentials from EEPROM
bool readWifiCredentials(String& ssid, String& password)
{
  bool success = true;
  String og_ssid = ssid, og_pass = password;
  success &= Preferences.begin("credentials", true);
  ssid = Preferences.getString("ssid", ssid);
  password = Preferences.getString("pass", password);
  // compare against default values
  success &= (ssid != og_ssid && password != og_pass);
  // verify ssid and password character length according to standard limits
  success &= (ssid.length() <= WIFI_SSID_LIMIT_MAX, password.length() > WIFI_PASS_LIMIT_MIN, password.length() <= WIFI_PASS_LIMIT_MAX);
  Preferences.end();
  if (!success)
    Serial.println("Failed to extract existing wifi credentials");
  return success;
}

// clear EEPROM
bool clearWifiCredentials()
{
  bool success = true;
  success &= Preferences.begin("credentials", false);
  success &= Preferences.remove("ssid");
  success &= Preferences.remove("pass");
  Preferences.end();
  if (success)
  {
    Serial.println("Successfully cleared wifi credentials!");
  }
  else
  {
    Serial.println("Failed to clear existing wifi credentials");
  }
  return success;
}

void handleEEPROMResetBtn()
{
  bool state = !digitalRead(RESETPin);
  if (state)
  {
    if (!resetSwitchTicker.active())
    {
      resetSwitchTicker.once(2.0, []{
        // clear wifi credentials
        if (clearWifiCredentials())
        {
          Serial.println("Restarting ESP board ...");
          ESP.restart();
        }
      });
    }
  }
  else
  {
    resetSwitchTicker.detach();
  }
}

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

  STAServer.on("/LEDstate", handleGetLEDState);

  STAServer.on("/toggleLED", handleToggleLED);

  // handle not found
  handleNotFound(STAServer);
}

bool connectWifi(const String& ssid, const String& password)
{
  WiFi.disconnect(true, true); 
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

  if (status == WL_CONNECTED)
  {
    saveWiFiCredentials(ssid, password);
    STAConnected = true;
  }

  return status == WL_CONNECTED;
}

// Honestly no idea what happens here but sure hope this works
void startMDNS()
{
  if (!MDNS.begin(Hostname)) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
}
