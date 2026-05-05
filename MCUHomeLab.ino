#include <WiFi.h>
#include <WebServer.h>

#include <memory>
#include <sstream>

// MCU board constants

constexpr int NPNBasePin = 2;

// WiFi

WebServer HomeLabServer(80);

// AP

WebServer APServer(81);
constexpr const char* APSsid = "ESP32 Wifi";
constexpr const char* APPassword = "12345678";

// middleware

void handleNotFound(WebServer& server)
{
  server.onNotFound([&server]{
    server.send(
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
  if (APServer.hasArg("ssid") && APServer.hasArg("password"))
  {
    bool connected = connectWifi(APServer.arg("ssid"), APServer.arg("password"));
    
    if (connected)
    {
      const char* connectedSSID = WiFi.SSID().c_str(); // for some reason this returns special characters. TODO: inspect the raw buffer, perhaps a messed up byte conversion
      std::stringstream message;
      message << "<p>";
      message << "Successfully connected wifi with SSID: " << connectedSSID;
      message << " With IpAddress: " << WiFi.localIP().toString().c_str();
      message << "</p>";
      APServer.send(200, "text/html", writeHTML(message.str().c_str()));

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

  APServer.send(200, "text/html", writeHTML(content.str().c_str()));
}

void setup()
{
  Serial.begin(115200);

  // setup AP + AP Server
  startAPServer();
  startHomeLabServer();
}

void loop()
{
  APServer.handleClient();
  HomeLabServer.handleClient();

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

void startAPServer()
{
  if (WiFi.softAP(APSsid, APPassword))
  {
    Serial.printf("Started WiFi access point with SSID: %s\n", APSsid);

    APServer.begin();

    // handle root
    APServer.on("/", [&APServer]{
      APServer.send(
        200, "text/html", R"(
        <!DOCTYPE html>
        <html>
          <body>
            <p>Welcome to my ESP32 Board</p>
            <form action="/connect-wifi">
              <input type="submit" value="Configure Wifi"/>
            </form>
          </body>
        </html>)"
      );
    });

    // handle not found
    handleNotFound(APServer);

    APServer.on("/connect-wifi", handleWifiConnection);

    Serial.printf("Started APServer. Now listening at: %s\n", WiFi.softAPIP().toString());
  }
}

void startHomeLabServer()
{
  // need a more efficient way to handle web servers, ideally we do not want the full recreation
  delay(50);
  HomeLabServer.begin();

  Serial.printf("Started Home lab server at address: %s\n", WiFi.localIP().toString());

  // handle root
  HomeLabServer.on("/", [&HomeLabServer]{
    HomeLabServer.send(
      200, "text/html", R"(
      <!DOCTYPE html>
      <html>
        <body>
          <p>Welcome to caykay's Home lab</p>
        </body>
      </html>)"
    );
  });
  // handle not found
  handleNotFound(HomeLabServer);
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
