#include "sendemail.h"

SendEmail::SendEmail(String& host, int port, String& user, String& passwd, int timeout, bool ssl) :
    host(host), port(port), user(user), passwd(passwd), timeout(timeout), ssl(ssl), client((ssl) ? new WiFiClientSecure() : new WiFiClient())
{

}

String SendEmail::readClient()
{
  String r = client->readStringUntil('\n');
  r.trim();
  while (client->available()) r += client->readString();
  return r;
}

bool SendEmail::send(String& from, String& to, String& subject, String& msg)
{
  if (!host.length())
  {
    return false;
  }
  client->stop();
  client->setTimeout(timeout);
  // smtp connect
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.print("Connecting: ");
  DEBUG_EMAIL_PORT.print(host);
  DEBUG_EMAIL_PORT.print(":");
  DEBUG_EMAIL_PORT.println(port);
#endif
  if (!client->connect(host.c_str(), port))
  {
    DEBUG_EMAIL_PORT.println("Can't connect");
    return false;
  }
  String buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  if (!buffer.startsWith("220"))
  {
    DEBUG_EMAIL_PORT.println("Answer not 220");
    return false;
  }
  buffer = "EHLO ";
  buffer += client->localIP().toString();
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  if (!buffer.startsWith("250"))
  {
    DEBUG_EMAIL_PORT.println("Answer not 250");
    return false;
  }
  if (user.length()>0  && passwd.length()>0 )
  {
    buffer = "AUTH LOGIN";
    client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
    if (!buffer.startsWith("334"))
    {
      DEBUG_EMAIL_PORT.println("Answer not 334");
      return false;
    }
    base64 b;
    buffer = user;
    buffer = b.encode(buffer);
    client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
    if (!buffer.startsWith("334"))
    {
      DEBUG_EMAIL_PORT.println("Answer not 334");
      return false;
    }
    buffer = this->passwd;
    buffer = b.encode(buffer);
    client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
    if (!buffer.startsWith("235"))
    {
      DEBUG_EMAIL_PORT.println("Answer not 235");
      return false;
    }
  }
  // smtp send mail
  buffer = "MAIL FROM: ";
  buffer += from;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  if (!buffer.startsWith("250"))
  {
    DEBUG_EMAIL_PORT.println("Answer not 250");
    return false;
  }
  buffer = "RCPT TO: ";
  buffer += to;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  if (!buffer.startsWith("250"))
  {
    DEBUG_EMAIL_PORT.println("Answer not 250");
    return false;
  }
  buffer = "DATA";
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  if (!buffer.startsWith("354"))
  {
    DEBUG_EMAIL_PORT.println("Answer not 354");
    return false;
  }
  buffer = "From: ";
  buffer += from;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  buffer = "To: ";
  buffer += to;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  buffer = "Subject: ";
  buffer += subject;
  buffer += "\r\n";
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  buffer = msg;
  client->println(buffer);
  client->println('.');
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  buffer = "QUIT";
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  DEBUG_EMAIL_PORT.println(buffer);
#endif
  return true;
}
