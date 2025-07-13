/*
For Non-contact Water Liquid Level Sensor 
https://www.electrodragon.com/product/non-contact-liquid-level-sensor/

ESP: ELEGOO 3pcs ESP-WROOM-32 Development Board

5-12V: Input voltage (Sensor) -> 5v on esp32
Out: Output signal, output singal current 2mA (Sensor)  -> LLC high
GPIO 23 (D23) -> LLC low
GND: Ground 0V (Sensor) -> Ground on esp32
GND: Ground 0V (ESP) -> Ground on LLC
3.3v ESP -> 3.3v reference on LLC
M: Mode select out of output TTL signal -> not used
*/

#include <WiFiManager.h>
#include "secrets.h"
#include "discord.h"
#include "email.h"
#include <Preferences.h>
#define WATER_SENSOR 23
#define TRIGGER_PIN 0



// discord messages
String message = "This is a simple message!";
String embedJson = R"({
  "title": "Dehumidifier is full",
  "description": "Time to empty the Dehumidifier",
  "color": 16711680
})";

unsigned long lastLowTime = 0;
bool wasLow = false;
bool messageSent = false;
bool sensorReady = false;

const unsigned long sensorStabilizeTime = 5000;  // 5 seconds
unsigned long bootTime = 0;
String discord_webhook = "";
String author_email = "";
String app_password = "";
String recipient_email = "";
String recipient_email2 = "";


Preferences preferences;

void setupWifiManager() {
  WiFiManager wm;
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  if (digitalRead(TRIGGER_PIN) == LOW) {
    wm.resetSettings();
  }

  WiFiManagerParameter author_email_param("author_email", "Author Email", "", 200);
  WiFiManagerParameter app_password_param("app_password", "Gmail App Password", "", 200);
  WiFiManagerParameter recipient_email_param("recipient_email", "Recipment Email", "", 200); 
  WiFiManagerParameter recipient_email2_param("recipient_email2", "Optional Second Recipient Email", "", 200);

  WiFiManagerParameter discord_web_hook_param("discord_web_hook", "Discord Webhook", "", 200);

  wm.addParameter(&author_email_param);
  wm.addParameter(&app_password_param);
  wm.addParameter(&recipient_email_param);
  wm.addParameter(&recipient_email2_param);

  wm.addParameter(&discord_web_hook_param);

  bool res = wm.autoConnect("DehumidifierAP");
  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  } else {
    Serial.println("connected");
  }

  // Save user input to NVS (Preferences)
  preferences.begin("config", false);
  discord_webhook = discord_web_hook_param.getValue();
  author_email = author_email_param.getValue();
  app_password = app_password_param.getValue();
  recipient_email = recipient_email_param.getValue();
  recipient_email2 = recipient_email2_param.getValue();

  preferences.putString("webhook", discord_webhook);
  preferences.putString("author_email", author_email);
  preferences.putString("app_password", app_password);
  preferences.putString("recipient_email", recipient_email);
  preferences.putString("recipient_email2", recipient_email2);
  preferences.end();
}


void setup() {
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  
  WiFi.begin();
  Serial.print("Connecting to WiFi");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {  // 10s timeout
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ Failed to connect to WiFi. Rebooting...");
    delay(3000);
  if (digitalRead(TRIGGER_PIN) == LOW) {
    setupWifiManager();
  }
    ESP.restart();
  }

  Serial.println("✅ WiFi connected. IP: " + WiFi.localIP().toString());

  preferences.begin("config", true);  // read-only
  discord_webhook = preferences.getString("webhook", "");
  author_email = preferences.getString("author_email", "");
  app_password = preferences.getString("app_password", "");
  recipient_email = preferences.getString("recipient_email", "");
  recipient_email2 = preferences.getString("recipient_email2", "");
  preferences.end();

  bool hasEmailInfo = author_email.length() > 0 || app_password.length() > 0 || recipient_email.length() > 0;
  bool hasDiscordWebhook = discord_webhook.length() > 0;

  if (!hasEmailInfo && !hasDiscordWebhook) {
    setupWifiManager();  // Only trigger if no saved config exists
  }

  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(WATER_SENSOR, INPUT);
  bootTime = millis();
  Serial.println("Waiting 5 seconds for sensor to stabilize...");
}

void loop() {
  // Reboot if WiFi is lost
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ Lost WiFi connection. Rebooting...");
    delay(3000); // Let Serial message print
    ESP.restart();
  }

  // Re-enter WiFiManager config mode if TRIGGER_PIN is LOW
  if (digitalRead(TRIGGER_PIN) == LOW) {
    setupWifiManager();
  }

  // Wait for sensor stabilization
  if (!sensorReady && millis() - bootTime < sensorStabilizeTime) {
    delay(100);
    return;
  }
  sensorReady = true;

  int sensorValue = digitalRead(WATER_SENSOR);
  Serial.println(sensorValue);  // 0 = wet, 1 = dry

  if (sensorValue == LOW) {  // Sensor detects water
    if (!wasLow) {
      lastLowTime = millis();
      wasLow = true;
    } else if (millis() - lastLowTime >= 1000 && !messageSent) {
      Serial.println("Sensor High for 1s, sending message");

      // Conditionally send email or Discord alert
      bool hasEmailInfo = author_email.length() > 0 || app_password.length() > 0 || recipient_email.length() > 0;
      bool hasDiscordWebhook = discord_webhook.length() > 0;

      if (hasEmailInfo) {
        sendEmail();
      } else if (hasDiscordWebhook) {
        sendDiscordEmbeds(embedJson);
      } else {
        Serial.println("❌ No alert method configured.");
        setupWifiManager();
      }
      messageSent = true;  // Prevent resending while still high
    }
  } else {
    wasLow = false;
    messageSent = false;  // Allow new message when it goes high again
  }

  delay(500);
}