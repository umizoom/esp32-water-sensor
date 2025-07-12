#include <WiFi.h>
#include <WiFiClientSecure.h>

#define ENABLE_SMTP  // Required for SMTPClient
#define ENABLE_DEBUG // Optional: enables debug messages
#define READYMAIL_DEBUG_PORT Serial

#include <ReadyMail.h>

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465  // 465 = SSL, 587 = STARTTLS


extern String author_email;
extern String app_password;
extern String recipient_email;
extern String recipient_email2;



WiFiClientSecure ssl_client;
SMTPClient smtp(ssl_client);

void smtpCb(SMTPStatus status) {
  if (status.progress.available) {
    Serial.printf("Uploading: %s (%d%%)\n", status.progress.filename.c_str(), status.progress.value);
  } else {
    Serial.printf("[SMTP] %s\n", status.text.c_str());
  }
}

void sendEmail() {
  ssl_client.setInsecure();
  if (!smtp.connect(SMTP_HOST, SMTP_PORT, smtpCb, false)) { // true = SSL mode
    Serial.println("❌ Failed to connect to SMTP server");
    return;
  }

  if (!smtp.authenticate(author_email, app_password, readymail_auth_password)) {
    Serial.println("❌ Authentication failed");
    return;
  }

  SMTPMessage msg;

  // Basic headers
  msg.headers.add(rfc822_subject, "🚨 Dehumidifier is full");
  msg.headers.add(rfc822_from, "Dehumidifier Alert <" + author_email +  ">");
  
  msg.headers.add(rfc822_to, "<"+ recipient_email +">");
  String to_header = "<" + recipient_email + ">";
  if (recipient_email2.length() > 0) {
    msg.headers.add(rfc822_to, "<"+ recipient_email2 +">");
  }

  // Optional: add priority
  msg.headers.addCustom("Importance", "High");
  msg.headers.addCustom("X-MSMail-Priority", "High");
  msg.headers.addCustom("X-Priority", "1");

  // Email content
  msg.html.body("Dehumidifier is full!");

  // Send the message
  if (!smtp.send(msg)) {
    Serial.println("❌ Email failed to send");
  } else {
    Serial.println("✅ Email sent successfully");
  }

}
