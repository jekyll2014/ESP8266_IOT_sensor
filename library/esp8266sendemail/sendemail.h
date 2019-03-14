#ifndef __SENDEMAIL_H
#define __SENDEMAIL_H

#define DEBUG_EMAIL_PORT Serial

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>

class SendEmail
{
  private:
    const String host;
    const int port;
    const String user;
    const String passwd;
    const int timeout;
    const bool ssl;
    WiFiClient* client;
    String readClient();
  public:
   SendEmail(String& host, int port, String& user, String& passwd, int timeout, bool ssl);
   bool send(String& from, String& to, String& subject, String& msg);
   ~SendEmail() {client->stop(); delete client;}
};

#endif
