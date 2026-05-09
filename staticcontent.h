static const char *APIndexContent = R"(
  <!DOCTYPE html>
  <html>
    <body>
      <p>Welcome to my ESP32 Board</p>
      <form action="/connect-wifi">
        <input type="submit" value="Configure Wifi"/>
      </form>
    </body>
  </html>
)";

static const char *APWifiSetupContent = R"(
  <!DOCTYPE html>
  <html>
    <body>
     <p>Setup the STA wifi:</p>
     <form method='post' action='/connect-wifi'>
       <input name="ssid" type='text' placeholder='ssid' required/>
       <input name='password' type='password' placeholder='password' required/>
       <input type="submit" value="Connect"/>
     </form>
    <!-- error message placeholder -->
    <p>%s</p>
    </body>
  </html>
)";

static const char *APEndedContent = R"(
  <!DOCTYPE html>
  <html>
    <body>
      <p>Connected to Wifi. Closing ESP32 Wifi Access Point.</p>
    </body>
  </html>
)";

static const char *STAIndexContent = R"(
  <!DOCTYPE html>
  <html>
    <body>
      <p>Welcome to caykay's Home lab</p>
    </body>
  </html>
)";

static const char *NotFoundContent = R"(
  <!DOCTYPE html>
  <html>
    <body>
      <p>Page not found </p>
    </body>
  </html>
)";
