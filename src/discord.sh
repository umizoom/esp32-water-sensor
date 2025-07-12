#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "discordCert.h"

extern String discord_webhook; 
const String discord_tts = DISCORD_TTS;


WiFiMulti WiFiMulti;


void sendDiscord(String content, String embedJson) {
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    client -> setCACert(DISCORD_CERT);
    {
      HTTPClient https;
      if (https.begin(*client, discord_webhook)) {\
        https.setTimeout(5000);
        https.addHeader("Content-Type", "application/json");

      Serial.println("Before payload");
        String jsonPayload = "{\"content\":\"" + content + "\",\"tts\":" + discord_tts + ",\"embeds\": [" + embedJson + "]}";
        int httpCode = https.POST(jsonPayload);
      Serial.println("after POST");
        if (httpCode > 0) {
            Serial.println("HTTP code: " + String(httpCode));
        }
        https.end();
      }
    }

    delete client;
  }
}

void sendDiscordMessage(String content) {
  sendDiscord(content, "");
}

void sendDiscordEmbeds(String embeds) {
  sendDiscord("", embeds);
}
