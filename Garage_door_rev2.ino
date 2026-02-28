#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// ---------------- HARDWARE ----------------
#define RELAY_PIN 15
#define RESET_PIN 5

// ---------------- DEFAULT SETTINGS ----------------
String mainPW  = "garage123";
String guestPW = "";
String apSSID  = "PicoW_Garage_AP";
String apPW    = "12345678";

bool loggedIn = false;
bool isGuest  = false;
bool doorBusy = false;

WebServer server(80);

// ---------------- SETTINGS STORAGE (ArduinoJson 7.4.2) ----------------
void loadSettings() {
  if (!LittleFS.exists("/settings.json")) {
    Serial.println("No settings file, using defaults.");
    return;
  }

  File f = LittleFS.open("/settings.json", "r");
  if (!f) {
    Serial.println("Failed to open settings file.");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    Serial.println("JSON parse error, using defaults.");
    return;
  }

  mainPW  = doc["main_password"] | mainPW;
  guestPW = doc["guest_password"] | guestPW;
  apSSID  = doc["ap_ssid"] | apSSID;
  apPW    = doc["ap_password"] | apPW;
}

void saveSettings() {
  JsonDocument doc;

  doc["main_password"] = mainPW;
  doc["guest_password"] = guestPW;
  doc["ap_ssid"] = apSSID;
  doc["ap_password"] = apPW;

  File f = LittleFS.open("/settings.json", "w");
  if (!f) {
    Serial.println("Failed to write settings file.");
    return;
  }

  serializeJson(doc, f);
  f.close();
}

// ---------------- FACTORY RESET ----------------
void factoryReset() {
  for (int i=0; i<20; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }

  mainPW = "garage123";
  guestPW = "";
  apSSID = "PicoW_Garage_AP";
  apPW   = "12345678";

  saveSettings();

  loggedIn = false;
  isGuest = false;

  delay(2000);
}

// ---------------- DOOR TRIGGER ----------------
void triggerDoor() {
  doorBusy = true;
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(RELAY_PIN, HIGH);
  delay(1000);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  doorBusy = false;
}

// ---------------- MOBILE UI CSS ----------------
String css() {
  return R"rawliteral(
    <style>
      body {
        font-family: Arial, sans-serif;
        text-align: center;
        background: #f2f2f2;
        margin: 0;
        padding: 0;
      }
      h1 {
        font-size: 34px;
        margin-top: 25px;
        color: #222;
      }
      .btn {
        display: inline-block;
        padding: 22px 45px;
        margin: 25px auto;
        font-size: 30px;
        font-weight: bold;
        color: white;
        background: #0078ff;
        border-radius: 14px;
        text-decoration: none;
        width: 70%;
      }
      .btn:active {
        background: #005fcc;
      }
      .disabled {
        background: #999 !important;
        color: #666 !important;
        pointer-events: none;
        opacity: 0.6;
      }
      .smallbtn {
        display: inline-block;
        padding: 14px 25px;
        margin: 12px;
        font-size: 22px;
        background: #444;
        color: white;
        border-radius: 10px;
        text-decoration: none;
      }
      input {
        font-size: 24px;
        padding: 12px;
        width: 70%;
        margin: 20px auto;
        border-radius: 10px;
        border: 1px solid #aaa;
      }
      button {
        font-size: 26px;
        padding: 14px 40px;
        background: #0078ff;
        color: white;
        border-radius: 12px;
        border: none;
        margin-top: 20px;
      }
    </style>
  )rawliteral";
}

// ---------------- HTML PAGES ----------------
String loginPage() {
  return R"rawliteral(
  <html><head>)rawliteral" + css() + R"rawliteral(</head>
  <body>
    <h1>Login</h1>
    <form action="/login">
      <input name="pw" type="password" placeholder="Password"><br>
      <button>Login</button>
    </form>
  </body></html>
  )rawliteral";
}

String adminPage() {
  String disabledPress  = doorBusy ? "disabled" : "";
  String disabledDelete = (guestPW.length() == 0) ? "disabled" : "";

  return R"rawliteral(
  <html><head>)rawliteral" + css() + R"rawliteral(</head>
  <body>
    <h1>Garage Controller</h1>
  )rawliteral"
  + "<a class=\"btn " + disabledPress + "\" href=\"/press\">PRESS</a><br>" +
  R"rawliteral(
    <a class="smallbtn" href="/changepw">Change PW</a>
    <a class="smallbtn" href="/createguest">Create Guest PW</a>
  )rawliteral"
  + "<a class=\"smallbtn " + disabledDelete + "\" href=\"/deleteguest\">Delete Guest PW</a>"
  + R"rawliteral(
    <a class="smallbtn" href="/network">Network</a>
    <a class="smallbtn" href="/logout">Logout</a>
  </body></html>
  )rawliteral";
}

String guestPage() {
  String disabledPress = doorBusy ? "disabled" : "";

  return R"rawliteral(
  <html><head>)rawliteral" + css() + R"rawliteral(</head>
  <body>
    <h1>Guest Access</h1>
  )rawliteral"
  + "<a class=\"btn " + disabledPress + "\" href=\"/press\">PRESS</a><br>" +
  R"rawliteral(
    <a class="smallbtn" href="/logout">Logout</a>
  </body></html>
  )rawliteral";
}

String changePWPage() {
  return R"rawliteral(
  <html><head>)rawliteral" + css() + R"rawliteral(</head>
  <body>
    <h1>Change Password</h1>
    <form action="/changepw">
      <input name="pw" type="password" placeholder="New Password"><br>
      <button>Save</button>
    </form>
  </body></html>
  )rawliteral";
}

String guestPWPage() {
  return R"rawliteral(
  <html><head>)rawliteral" + css() + R"rawliteral(</head>
  <body>
    <h1>Guest Password</h1>
    <form action="/createguest">
      <input name="pw" type="password" placeholder="Guest Password"><br>
      <button>Save</button>
    </form>
  </body></html>
  )rawliteral";
}

String networkPage() {
  return R"rawliteral(
  <html><head>)rawliteral" + css() + R"rawliteral(</head>
  <body>
    <h1>Network Settings</h1>
    <form action="/network">
      <input name="ssid" placeholder="SSID"><br>
      <input name="pw" type="password" placeholder="Password"><br>
      <button>Save</button>
    </form>
  </body></html>
  )rawliteral";
}

String pressAck() {
  return R"rawliteral(
  <html><head>)rawliteral" + css() + R"rawliteral(</head>
  <body>
    <h1>Door Triggered</h1>
    <meta http-equiv='refresh' content='2;url=/'>
  </body></html>
  )rawliteral";
}

// ---------------- ROUTES ----------------
void handleRoot() {
  if (!loggedIn) {
    server.send(200, "text/html", loginPage());
    return;
  }
  if (isGuest) server.send(200, "text/html", guestPage());
  else server.send(200, "text/html", adminPage());
}

void handleLogin() {
  String pw = server.arg("pw");
  if (pw == mainPW) { loggedIn = true; isGuest = false; }
  else if (pw == guestPW) { loggedIn = true; isGuest = true; }
  else { loggedIn = false; isGuest = false; }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleLogout() {
  loggedIn = false;
  isGuest = false;
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handlePress() {
  triggerDoor();
  server.send(200, "text/html", pressAck());
}

void handleChangePW() {
  if (!loggedIn || isGuest) return handleRoot();

  if (server.hasArg("pw")) {
    mainPW = server.arg("pw");
    saveSettings();
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
    return;
  }

  server.send(200, "text/html", changePWPage());
}

void handleGuestPW() {
  if (!loggedIn || isGuest) return handleRoot();

  if (server.hasArg("pw")) {
    guestPW = server.arg("pw");
    saveSettings();
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
    return;
  }

  server.send(200, "text/html", guestPWPage());
}

void handleDeleteGuest() {
  if (!loggedIn || isGuest) return handleRoot();

  guestPW = "";
  saveSettings();

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleNetwork() {
  if (!loggedIn || isGuest) return handleRoot();

  if (server.hasArg("ssid") && server.hasArg("pw")) {
    apSSID = server.arg("ssid");
    apPW   = server.arg("pw");
    saveSettings();

    server.send(200, "text/html",
      "<html><body><h1>Saved</h1>"
      "<p>Rebooting...</p>"
      "<meta http-equiv='refresh' content='1;url=/'></body></html>"
    );

    delay(1000);
    rp2040.reboot();
    return;
  }

  server.send(200, "text/html", networkPage());
}

// ---------------- WIFI AP ----------------
void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID.c_str(), apPW.c_str());
  delay(500);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP started: ");
  Serial.println(ip);
}

// ---------------- SETUP ----------------
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(1000);

  LittleFS.begin();
  loadSettings();

  startAP();

  server.on("/", handleRoot);
  server.on("/login", handleLogin);
  server.on("/logout", handleLogout);
  server.on("/press", handlePress);
  server.on("/changepw", handleChangePW);
  server.on("/createguest", handleGuestPW);
  server.on("/deleteguest", handleDeleteGuest);
  server.on("/network", handleNetwork);

  server.begin();
}

// ---------------- LOOP ----------------
void loop() {
  if (digitalRead(RESET_PIN) == LOW) {
    unsigned long start = millis();
    Serial.println("Hold reset 5s...");

    while (digitalRead(RESET_PIN) == LOW) {
      unsigned long elapsed = millis() - start;
      if (elapsed >= 5000) {
        Serial.println("Factory reset!");
        factoryReset();
        startAP();
        break;
      }
      delay(100);
    }
  }

  server.handleClient();
}