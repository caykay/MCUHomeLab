#include <WiFi.h>
#include <WebServer.h>

#include <memory>
#include <sstream>

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
    server->send(
      404, "text/html", R"(
      <!DOCTYPE html>
      <html>
        <body>
          <p>Page not found </p>
        </body>
      </html>)"
    );
  });
}

void handleWifiConnection()
{
  std::string errorMsg = "";
  if (APServer->hasArg("ssid") && APServer->hasArg("password"))
  {
    auto ssid = APServer->arg("ssid");
    auto password = APServer->arg("password");

    Serial.print("Attempting to connect to wifi -> SSID: ");
    Serial.println(ssid);

    WiFi.disconnect(true, true); 

    WiFi.mode(WIFI_AP_STA); // allows the esp32 to act both as a AP and STA
    auto status = WiFi.begin(ssid, password);
    int attempts = 8;
    while (status != WL_CONNECTED && attempts > 1)
    {
      delay(500);
      status = WiFi.status();
      attempts --;
      Serial.print('.');
    }

    Serial.println("");
    
    if (status == WL_CONNECTED)
    {
      const char* connectedSSID = WiFi.STA.SSID().c_str();
      // debug logs
      Serial.print("Connected to wifi with SSID: ");
      Serial.print(connectedSSID );
      Serial.print(" and IP address: ");
      Serial.println(WiFi.localIP().toString());

      std::stringstream message;
      message << "<p>";
      message << "Successfully connected wifi with SSID: " << connectedSSID;
      message << " With IpAddress: " << WiFi.localIP().toString().c_str();
      message << "</p>";
      // debug code only
      APServer->send(200, "text/html", writeHTML(message.str().c_str()));
      // TODO -> start the Public Web Server
      return;
    }
    else
    {
      Serial.println("Could not connect to the wifi");
      errorMsg = "Failed to connect to the wifi. Try again";
    }
  }

  std::stringstream content;
  content << R"(
    <p>Configure the Home Lab wifi:</p>
    <form method='post' action='/connect-wifi'>
      <input name="ssid" type='text' placeholder='ssid' required/>
      <input name='password' type='password' placeholder='password' required/>
      <input type="submit" value="Connect"/>
    </form>
  )";
  content << "<p>" << errorMsg << "</p>";

  APServer->send(200, "text/html", writeHTML(content.str().c_str()));
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
      APServer->send(
        200, "text/html", R"(
        <!DOCTYPE html>
        <html>
          <body>
            <p>Welcome to my ESP32 Board</p>
            <form action="/wifi-setup">
              <input type="submit" value="Configure Wifi"/>
            </form>
          </body>
        </html>)"
      );
    });

    // handle not found
    handleNotFound(APServer);

    APServer->on("/connect-wifi", handleWifiConnection);

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

  if (PublicServer != nullptr)
    PublicServer->handleClient();

  delay(2);
}

// helper methods

const char* writeHTML(const char* bodyContent)
{
  std::stringstream htmlContent;
  htmlContent << R"(
    <!DOCTYPE html>
    <html>
    <body>)";
  htmlContent << bodyContent;
  htmlContent << R"(
    </body>
    </html>)";
  
  return htmlContent.str().c_str();
}
