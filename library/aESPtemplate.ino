//                    arduinoESP8266 wifi & eeprom setting template
// ----------------------------------- libs -----------------------------------
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <EEPROM.h>


// ----------------------------------- vars -----------------------------------
#define MODULE_DESCRIPTION "arduinoESP8266 wifi & eeprom setting template"
#define DEFAULT_STA_ENABLED false               // режим клиента (station) 
#define DEFAULT_DISABLE_AP false                // выключать режим точки доступа (AP) при подключенном режиме клиента
#define DEFAULT_SENDER_ENABLED false            // отправка данных GET запросом
#define DEFAULT_HTTP_AUTH_ENABLED false         // защищать http авторизацией страницу настроек при входе через ip клиента (STA)

#define CONFIG_HTTP_USER_DEFAULT "123"
#define CONFIG_HTTP_PASSWORD_DEFAULT "321"

#define STA_SSID_DEFAULT "CLIENTSSID"
#define STA_PASSWORD_DEFAULT "WiFinetKEY"

#define NAME_TAG "ESP"

#define AP_SSID NAME_TAG "8266-"
#define AP_PASSWORD_PREFIX "SECRET" // + mac XXXX
#define AP_PASSWORD_SUFFIX ""

// ----------------------------------- web&ota
ESP8266WebServer server(80);

#define URL_PORT 80
#define URL_HOST "test.lan"
#define URL_PATH "/info/"
#define URL_STORE URL_PATH "store.php"

#define OTAWEB_BUILD_VERSION "20161030_1"

#define OTAWEB_MODULE_TYPE "esp_template_ota"
#define OTAWEB_PORT 80
#define OTAWEB_HOST "http://" URL_HOST
#define OTAWEB_URL OTAWEB_HOST "/espupd/esp.php?espfirmware=" OTAWEB_MODULE_TYPE
#define OTAWEB_URL_INFO OTAWEB_HOST "/espupd/esp.php?info=true&espfirmware=" OTAWEB_MODULE_TYPE


// ----------------------------------- common
#define senderTickTime 1000*60*5
#define serialTickTime 1000
#define apCheckTickTime 60000
#define sensTickTime 30000


bool rebootReq = false;
unsigned long nextSenderTick;
unsigned long nextSerialTick;
unsigned long nextapCheckTick;
unsigned long nextSensTick;

unsigned long staConnectTime = 0;

// ----------------------------------- wifi
IPAddress ipSTA;
IPAddress ipAP;
bool staInitOk;
String checkClientNetStr;
String APmacStr;
String APmacID;
String STAmacStr;
String STAmacID;
String ipAPstr;
String ipSTAstr;
char AP_NameChar[32];
char AP_PassChar[32];


// ----------------------------------- eeprom

#define EEPROM_START 0
boolean setEEPROM = false;
uint32_t memcrc; uint8_t *p_memcrc = (uint8_t*)&memcrc;

struct eeprom_data_t {
  bool senderEnabled;
  bool STAenabled;
  bool disableAP;
  bool CONFIGauthEnabled;
  char STAssid[17];
  char STApass[17];
  char CONFIGuser[10];
  char CONFIGpass[10];
} eeprom_data;

//static PROGMEM prog_uint32_t crc_table[16] = {
static  prog_uint32_t crc_table[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};


// ----------------------------------- setup -----------------------------------
void setup() {

  Serial.begin(115200);
  Serial.println(MODULE_DESCRIPTION);
  Serial.println(OTAWEB_BUILD_VERSION);
  Serial.print("SDK version: "); Serial.println(ESP.getSdkVersion());
  Serial.print("Flash real size: "); Serial.println(ESP.getFlashChipRealSize());
  Serial.print("Firmware compiled for flash: "); Serial.println(ESP.getFlashChipSize());
  delay(10);


  readSettingsESP();




  WiFi.disconnect(true);
  WiFi.persistent(false);
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  delay(500);
  WiFi.onEvent(WiFiEvent);



  if (eeprom_data.STAenabled == true )
  {
    Serial.print("STA enabled, checking : ");  Serial.println(eeprom_data.STAssid);

    if (checkAPinair(String(eeprom_data.STAssid)))
    {
      WiFi.mode(WIFI_AP_STA);
      softAPinit();
      STAinit();
      Serial.print(millis() / 1000); Serial.println(": AP found, AP_STA mode");
    }
    else
    {
      WiFi.mode(WIFI_AP);
      softAPinit();
      Serial.print(millis() / 1000); Serial.println(": AP not found, AP mode");
    }
  }

  else
  {
    WiFi.mode(WIFI_AP);
    softAPinit();
  }


  server.on("/", handleRoot);
  server.on("/a", handleActions);
  server.on("/c", handleConfig);
  //server.on("/reboot", handleReboot);
  //server.on("/serialcheck", handleSerialCheck);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not Found\n\n");
  });

  server.begin();

  nextSensTick = millis();
  nextSerialTick =  millis() ;
  nextSenderTick = 20000 + millis() ;

}

// ----------------------------------- loop -----------------------------------
void loop() {


  processSensors();
  processAPcheck();

  processURL();
  processSerial();

  processReboot();

  server.handleClient();
}
// ----------------------------------- processReboot -----------------------------------
void processReboot() {
  if (rebootReq == true)
  {

    delay(1000);
    ESP.reset();

  }

}
// ----------------------------------- wifiEvent -----------------------------------
void WiFiEvent(WiFiEvent_t event) {
  //Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event) {
    case WIFI_EVENT_STAMODE_GOT_IP:
      staConnectTime = millis();
      if (staInitOk == false)
      {
        staInitOk = true;
        nextSenderTick =  millis() + 30000;

        ipSTA = WiFi.localIP();
        ipSTAstr = String(ipSTA[0]) + '.' + String(ipSTA[1]) + '.' + String(ipSTA[2]) + '.' + String(ipSTA[3]);

        if (eeprom_data.disableAP == true)
        {
          WiFi.mode(WIFI_STA);
        }

        //Serial.print(millis() / 1000); Serial.println(": STA connected");

        Serial.print(millis() / 1000); Serial.print(": STA IP address: "); Serial.println(ipSTAstr);

        /*if (eeprom_data.senderEnabled == true)
          {
          String url = URL_STORE;
          url += "?log=init";
          url += "&text=i:";
          url += STAmacID;
          url += ",freeram:";
          url += ESP.getFreeHeap();
          url += ",locip:";
          url += ipSTAstr;
          url += ",uptime:";
          url += millis() / 1000;
          url += ",rssi:";
          url += String(WiFi.RSSI());

          sendURL (url);
          }*/
      }
      break;
    case WIFI_EVENT_STAMODE_DISCONNECTED:
      if (staInitOk == true)
      {
        staInitOk = false;
        if (eeprom_data.disableAP == true)
        {
          WiFi.mode(WIFI_AP_STA);
        }
      }

      //Serial.println("WiFi lost connection");
      break;


      /*WIFI_EVENT_STAMODE_CONNECTED = 0,
        WIFI_EVENT_STAMODE_DISCONNECTED,1
        WIFI_EVENT_STAMODE_AUTHMODE_CHANGE,2
        WIFI_EVENT_STAMODE_GOT_IP,3
        WIFI_EVENT_STAMODE_DHCP_TIMEOUT,4
        WIFI_EVENT_SOFTAPMODE_STACONNECTED,5
        WIFI_EVENT_SOFTAPMODE_STADISCONNECTED,6
        WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED,7
        WIFI_EVENT_MAX*/
  }
}
// ----------------------------------- checkAPinair -----------------------------------
boolean checkAPinair( String name) {
  name.toUpperCase();

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  //WiFi.mode(WIFI_STA);
  //WiFi.disconnect();
  //Serial.println(name);

  int n = WiFi.scanNetworks();
  //Serial.println("scan done");
  if (n == 0)
  {
    //Serial.println("no networks found");
    return false;
  }

  else
  {
    String nnn;
    //Serial.print(n);    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      nnn = WiFi.SSID(i);
      nnn.toUpperCase();
      //Serial.println(nnn);

      if (nnn == name)
      {
        //Serial.print(name);    Serial.print(" network found, RSSI:"); Serial.println(WiFi.RSSI(i));;
        return true;
      }
    }
  }
  return false;

}
// ----------------------------------- processAPcheck -----------------------------------
void processAPcheck() {
  if (eeprom_data.STAenabled == true && staInitOk == false)
  {
    if (nextapCheckTick <= millis())
    {
      nextapCheckTick =  millis() + apCheckTickTime;






      Serial.print("STA not connected, checking ssid : ");  Serial.println(eeprom_data.STAssid);

      if (checkAPinair(String(eeprom_data.STAssid)))
      {
        WiFi.mode(WIFI_AP_STA);

        //WiFi.softAP(AP_NameChar, AP_PassChar);
        WiFi.begin(eeprom_data.STAssid, eeprom_data.STApass);
        //softAPinit();
        //STAinit();
        Serial.print(millis() / 1000); Serial.println(": AP found, AP_STA mode");
      }
      else
      {
        WiFi.mode(WIFI_AP);
        // WiFi.softAP(AP_NameChar, AP_PassChar);
        //softAPinit();
        Serial.print(millis() / 1000); Serial.println(": AP not found, AP mode");
      }


    }
  }
}

// ----------------------------------- STAinit -----------------------------------
void STAinit() {

  WiFi.begin(eeprom_data.STAssid, eeprom_data.STApass);


  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);



  STAmacStr = String(mac[WL_MAC_ADDR_LENGTH - 6], HEX) + ":" +
              String(mac[WL_MAC_ADDR_LENGTH - 5], HEX) + ":" +
              String(mac[WL_MAC_ADDR_LENGTH - 4], HEX) + ":" +
              String(mac[WL_MAC_ADDR_LENGTH - 3], HEX) + ":" +
              String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + ":" +
              String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);


  STAmacID = "";

  if (mac[WL_MAC_ADDR_LENGTH - 2] < 0x10)  STAmacID = STAmacID + "0";
  STAmacID = STAmacID + String(mac[WL_MAC_ADDR_LENGTH - 2], HEX);

  if (mac[WL_MAC_ADDR_LENGTH - 1] < 0x10)  STAmacID = STAmacID + "0";
  STAmacID = STAmacID + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);



  STAmacStr.toUpperCase();
  STAmacID.toUpperCase();

  Serial.print("STA MAC address: ");
  Serial.println(STAmacStr);

  Serial.print("STA SSID: ");
  Serial.println(eeprom_data.STAssid);


}
// ----------------------------------- softAPinit -----------------------------------
void softAPinit() {

  //Serial.println();

  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac)     ;



  APmacStr = String(mac[WL_MAC_ADDR_LENGTH - 6], HEX) + ":" +
             String(mac[WL_MAC_ADDR_LENGTH - 5], HEX) + ":" +
             String(mac[WL_MAC_ADDR_LENGTH - 4], HEX) + ":" +
             String(mac[WL_MAC_ADDR_LENGTH - 3], HEX) + ":" +
             String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + ":" +
             String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);


  APmacID = "";

  if (mac[WL_MAC_ADDR_LENGTH - 2] < 0x10)  APmacID = APmacID + "0";
  APmacID = APmacID + String(mac[WL_MAC_ADDR_LENGTH - 2], HEX);

  if (mac[WL_MAC_ADDR_LENGTH - 1] < 0x10)  APmacID = APmacID + "0";
  APmacID = APmacID + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);


  APmacStr.toUpperCase();
  APmacID.toUpperCase();


  String AP_NameString = AP_SSID + APmacID;

  //char AP_NameChar[AP_NameString.length() + 1];
  //char AP_PassChar[AP_PassString.length() + 1];

  for (int i = 0; i < AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);
  AP_NameChar[AP_NameString.length() ] = 0;


  String AP_PassString = AP_PASSWORD_PREFIX + APmacID + AP_PASSWORD_SUFFIX;

  for (int i = 0; i < AP_PassString.length(); i++)
    AP_PassChar[i] = AP_PassString.charAt(i);
  AP_PassChar[AP_PassString.length() ] = 0;

  WiFi.softAP(AP_NameChar, AP_PassChar);
  Serial.print("AP SSID: ");  Serial.println(AP_NameChar);
  //Serial.print("AP pass: ");  Serial.println(AP_PassChar);

  ipAP = WiFi.softAPIP();

  ipAPstr = String(ipAP[0]) + '.' + String(ipAP[1]) + '.' + String(ipAP[2]) + '.' + String(ipAP[3]);
  checkClientNetStr = String(ipAP[0]) + '.' + String(ipAP[1]) + '.' + String(ipAP[2]) + '.' ;

  Serial.print("AP IP address: ");
  Serial.println(ipAPstr);

  Serial.print("AP MAC address: ");
  Serial.println(APmacStr);


}
// ----------------------------------- processSerial -----------------------------------
void processSerial(void) {


  if (nextSerialTick <= millis())
  {
    nextSerialTick =  millis() + serialTickTime;

    String message;

    while (Serial.available())
    {
      message = message + Serial.readString();
    }

    //Serial.print( message );

  }
}
// ----------------------------------- processSensors -----------------------------------
void processSensors ()
{
  if (nextSensTick <= millis())
  {
    nextSensTick = millis() + sensTickTime;



  }

}
// ----------------------------------- processURL -----------------------------------
void processURL() {

  if (eeprom_data.senderEnabled == true && staInitOk == true && nextSenderTick <= millis())
  {
    nextSenderTick =  millis() + senderTickTime;

    String url = URL_STORE;
    url += "?i=";
    url += STAmacID;
    url += "&uptime=";
    url += millis() / 1000;
    url += "&freeram=";
    url += ESP.getFreeHeap();
    url += "&rssi=";
    url += String(WiFi.RSSI());



    sendURL(url);

  }
}
// ----------------------------------- sendURL -----------------------------------
void sendURL(String url) {

  WiFiClient client;

  if (!client.connect(URL_HOST, URL_PORT)) {
    Serial.print(millis() / 1000); Serial.println(": connection failed");
    return;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + URL_HOST + "\r\n" +
               "Connection: close\r\n" +
               "User-Agent: arduinoESP8266\r\n" +
               "\r\n");
  delay(5);

  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.print(line);
    yield();
  }

}
// ----------------------------------- humanTimeMillis -----------------------------------
String humanTimeMillis(unsigned long milli) {

  String s;

  //unsigned long milli;
  //milli = millis();

  unsigned long secs = milli / 1000, mins = secs / 60;
  unsigned int hours = mins / 60, days = hours / 24;
  milli -= secs * 1000;
  secs -= mins * 60;
  mins -= hours * 60;
  hours -= days * 24;
  s += days != 0 ?  (String)days : "";
  s += days != 0 ?  "d " : "";
  s += hours != 0 ?  (String)hours : "";
  s += hours != 0 ?  ":" : "";
  s += mins > 9 ?  "" : "0";
  s += mins;
  s += ":";
  s += secs > 9 ?  "" : "0";
  s += secs;
  /*s += ".";
    if (milli < 10)    s += "00";  else if (milli < 100)    s += "0";
    s += milli;*/

  return s;
}

// ----------------------------------- handleRoot -----------------------------------
void handleRoot() {


  String s = "<!DOCTYPE HTML>\r\n<html><head><meta charset=\"utf-8\"><title>" + String(NAME_TAG) + "8266_";

  if (APmacID.length() > 0)
    s += APmacID;
  else
    s += STAmacID;
  s += "</title></head>";

  s += "Status - <a href='/a'>Actions</a> - <a href='/c'>Config</a>";
  s += "<br>";

  s += "<br>";
  s += MODULE_DESCRIPTION;
  s += "<br>";

  s += "Flash real size: ";
  s += ESP.getFlashChipRealSize();
  s += "<br>";
  s += "Firmware compiled for flash: ";
  s += ESP.getFlashChipSize();
  s += "<br>";

  s += "<br>STA connect time: ";
  if (staConnectTime == 0)
  {
    s += "no data";
  } else {
    s += humanTimeMillis(millis() - staConnectTime);
    s += " ago";
  }
  s += "<br>";

  if (eeprom_data.STAenabled == true && WiFi.status() != WL_CONNECTED)
    s += "<br>wifi client enabled and disconnected";
  else if (eeprom_data.STAenabled == true && WiFi.status() == WL_CONNECTED)
  {
    s += "<br>wifi client connected";
    if (ipSTAstr.length() > 0)
    {
      s += ", ip address: <a href=\"http://";
      s += ipSTAstr;
      s += "\">";
      s += ipSTAstr;
      s += "</a>";

      s += ", RSSI: ";
      s += String(WiFi.RSSI());
      s += " dBm";
    }

  }
  else
    s += "<br>wifi client disabled";


  if (eeprom_data.senderEnabled == true)
    s += "<br>sender enabled";
  else
    s += "<br>sender disabled";

  s += ", url http://";
  s += URL_HOST;
  s += URL_STORE;

  s += "<br>OTAWEB update url: ";
  s += OTAWEB_URL;
  s += ", build version: ";
  s += OTAWEB_BUILD_VERSION;

  s += "<br>";
  s += "<br>uptime: ";

  s += humanTimeMillis(millis());



  s += "<br>";
  s += "<br>EEPROM at boot: ";
  s += setEEPROM ? "ok" : "fail";


  /*s += "<table width=\"100%\"><tr><td align=\"right\">";
    s += "<small>dreamer2 <a href=\"skype:dreamer.two?chat\">Skype</a></small>";
    s += "</td></tr></table>";*/

  s += "</html>\r\n\r\n";

  server.send(200, "text/html", s);
}
// ----------------------------------- handleActions -----------------------------------
void handleActions() {
  String s = "<!DOCTYPE HTML>\r\n<html><head><meta charset=\"utf-8\"><title>" + String(NAME_TAG) + "8266_";

  if (APmacID.length() > 0)
    s += APmacID;
  else
    s += STAmacID;
  s += "</title></head>";
  s += "<a href='/'>Status</a> - Actions - <a href='/c'>Config</a>";
  s += "<br><br><a href='/a?reboot=true'>Reboot</a>";
  s += "<br><br><a href='/a?serialcheck=true'>Serialcheck</a>";

  s += "<br>";

  s += "<br><a href='/a?otaupdateinfo=true'>OTAWEB firmware check</a>";

  if (ESP.getFlashChipRealSize() > 900000)
  {
    s += "<br><font color=\"red\">Check your new firmware compile time size! must be 1mbyte+</font>";
  }
  else
  {
    s += "<br><font color=\"red\">Your flash ";
    s += String(ESP.getFlashChipRealSize());
    s += " bytes only, it's too small for OTA WEB</font>";
  }
  //s += "<br><a href='/a&otaupdate'>OTAWEB firmware update</a>";

  s += "<br>";
  s += "module type: ";
  s += OTAWEB_MODULE_TYPE;
  s += "<br>";
  s += "module fw: ";
  s += OTAWEB_BUILD_VERSION;


  if (   staInitOk == true && server.hasArg("otaupdateinfo"))
  {
    s += "<br>";
    s += "server fw: ";

    HTTPClient http;
    http.begin(OTAWEB_URL_INFO);
    int httpCode = http.GET();
    //Serial.println(httpCode);
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      //USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        // Serial.println(payload);
        if (payload.length() > 0 && payload != "firmware type error")
        {
          s += payload;
          if (payload != OTAWEB_BUILD_VERSION)         s += "<br><a href='/a?otaupdate=true'>OTAWEB firmware update</a>";
        }
        else
          s += "server error";
      }
    } else {
      //USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      s += "[HTTP] GET... failed";
    }

    http.end();
  }
  else if (  staInitOk == true && server.hasArg("otaupdate"))
  {
    Serial.println("OTAWEB update request");

    t_httpUpdate_return ret = ESPhttpUpdate.update(OTAWEB_URL, OTAWEB_BUILD_VERSION);

    s += "<br>";
    s += "<br>";

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        s += "HTTP_UPDATE_FAILD Error (";
        s += ESPhttpUpdate.getLastError();
        s += "): ";
        s += ESPhttpUpdate.getLastErrorString().c_str();
        break;

      case HTTP_UPDATE_NO_UPDATES:
        s += "HTTP_UPDATE_NO_UPDATES";
        break;

      case HTTP_UPDATE_OK:
        s += "HTTP_UPDATE_OK";
        break;
    }
  }
  else if (   server.hasArg("serialcheck"))
  {
    Serial.println("serial checkcheckcheckcheckcheckcheckcheckcheckcheckcheckcheck");
    s = "HTTP/1.1 307 Temporary Redirect";
    s += "\r\nLocation: /a";
    s += "\r\n\r\n";
    server.sendContent(s);
    return;
  }
  else if (   server.hasArg("reboot"))
  {
    s = "<head>";
    s += "<meta http-equiv=\"refresh\" content=\"20;url=/\">";
    s += "</head>";
    s += "REDIRECTING in 20S";


    s += "<br><br>AP address: <a href=\"http://";
    s += ipAPstr;
    s += "\">";
    s += ipAPstr;
    s += "</a>";

    if (ipSTAstr.length() > 0)
    {
      s += ", STA address: <a href=\"http://";
      s += ipSTAstr;
      s += "\">";
      s += ipSTAstr;
      s += "</a>";
    }

    s += "</html>\r\n\r\n";

    rebootReq = true;
  }






  s += "</html>\r\n\r\n";
  server.send(200, "text/html", s);
}
// ----------------------------------- handleConfig -----------------------------------
void handleConfig() {


  /*Serial.print("Arguments: "); Serial.println(server.args());
    for (uint8_t i = 0; i < server.args(); i++) {
    Serial.print(server.argName(i));  Serial.print(" : "); Serial.println(server.arg(i));
    }
  */


  String s ;

  if (eeprom_data.CONFIGauthEnabled == true)
  {
    if (!server.authenticate(eeprom_data.CONFIGuser, eeprom_data.CONFIGpass))
      return server.requestAuthentication();
  }

  if (server.hasArg("ssid")  && server.hasArg("CONFIGuser")  &&  server.hasArg("CONFIGpass")  &&
      server.hasArg("STAenabled") && server.hasArg("senderEnabled")
      && server.hasArg("disableAP") && server.hasArg("CONFIGauthEnabled"))
  {
    Serial.println("config change request");

    if (server.arg("senderEnabled") == "true")
      eeprom_data.senderEnabled = true;
    else
      eeprom_data.senderEnabled = false;

    if (server.arg("STAenabled") == "true")
      eeprom_data.STAenabled = true;
    else
      eeprom_data.STAenabled = false;

    if (server.arg("disableAP") == "true")
      eeprom_data.disableAP = true;
    else
      eeprom_data.disableAP = false;

    if (server.arg("CONFIGauthEnabled") == "true")
      eeprom_data.CONFIGauthEnabled = true;
    else
      eeprom_data.CONFIGauthEnabled = false;

    String lPASS;
    lPASS = server.arg("pass");

    server.arg("ssid").toCharArray(eeprom_data.STAssid, sizeof(eeprom_data.STAssid));
    lPASS.toCharArray(eeprom_data.STApass, sizeof(eeprom_data.STApass));

    server.arg("CONFIGuser").toCharArray(eeprom_data.CONFIGuser, sizeof(eeprom_data.CONFIGuser));
    server.arg("CONFIGpass").toCharArray(eeprom_data.CONFIGpass, sizeof(eeprom_data.CONFIGpass));

    writeSettingsESP();


    if (server.arg("rebootRq") == "on")
    {

      s += "<head>";
      s += "<meta http-equiv=\"refresh\" content=\"20;url=/c\">";
      s += "</head>";
      s += "REDIRECTING in 20S";

      s += "<br><br>AP address: <a href=\"http://";
      s += ipAPstr;
      s += "\">";
      s += ipAPstr;
      s += "</a>";

      if (ipSTAstr.length() > 0)
      {
        s += ", STA address: <a href=\"http://";
        s += ipSTAstr;
        s += "\">";
        s += ipSTAstr;
        s += "</a>";
      }

      s += "</html>\r\n\r\n";


      rebootReq = true;
    }
    else
    {
      //STAinit();
      s = "HTTP/1.1 307 Temporary Redirect";
      s += "\r\nLocation: /c";
      s += "\r\n\r\n";

    }
    server.sendContent(s);
    //client.print(s);
    return;
  }

  //boolean clientConfigAllowed, isAPclient;

  //IPAddress remoteIP = client.remoteIP();
  //String remoteNETstr = String(remoteIP[0]) + '.' + String(remoteIP[1]) + '.' + String(remoteIP[2]) + '.' ;

  /*if (checkClientNetStr.length() > 0 && checkClientNetStr == remoteNETstr)
    isAPclient = true;
    else
    isAPclient = false;

    if (STA_WIFI_CONFIG_ENABLE == true || eeprom_data.CONFIGauthEnabled == true || isAPclient == true)
    clientConfigAllowed = true;
    else
    clientConfigAllowed = false;*/



  s = "<!DOCTYPE HTML>\r\n<html><head><meta charset=\"utf-8\"><title>" + String(NAME_TAG) + "8266_";

  if (APmacID.length() > 0)
    s += APmacID;
  else
    s += STAmacID;
  s += "</title></head>";

  s += "<a href='/'>Status</a> - <a href='/a'>Actions</a> - Config";


  s += "<br><form>";

  s += "<br>senderEnabled: <select name=\"senderEnabled\">";
  if (eeprom_data.senderEnabled == true)
  {
    s += "<option value =\"true\" selected=\"selected\">Enabled</option>";
    s += "<option value =\"false\">Disabled</option>";
  }
  else
  {
    s += "<option value =\"true\">Enabled</option>";
    s += "<option value =\"false\" selected=\"selected\">Disabled</option>";
  }
  s += "</select>";

  s += "<br>STAenabled: <select name=\"STAenabled\">";
  if (eeprom_data.STAenabled == true)
  {
    s += "<option value =\"true\" selected=\"selected\">Enabled</option>";
    s += "<option value =\"false\">Disabled</option>";
  }
  else
  {
    s += "<option value =\"true\">Enabled</option>";
    s += "<option value =\"false\" selected=\"selected\">Disabled</option>";
  }
  s += "</select>";


  s += "<br>disable AP when STA connected: <select name=\"disableAP\">";
  if (eeprom_data.disableAP == true)
  {
    s += "<option value =\"true\" selected=\"selected\">Enabled</option>";
    s += "<option value =\"false\">Disabled</option>";
  }
  else
  {
    s += "<option value =\"true\">Enabled</option>";
    s += "<option value =\"false\" selected=\"selected\">Disabled</option>";
  }
  s += "</select>";

  s += "<br>";


  s += "<br>ssid: <input type=text name=ssid size=30 maxlength=16 value='";
  s += eeprom_data.STAssid;

  s += "' />";
  //if (clientConfigAllowed == true)
  //{
  s += "<br>pass: <input type=text name=pass size=30 maxlength=16 value='";
  s += eeprom_data.STApass;
  s += "' /><br>";
  /*}
    else
    {
    s += "<br>pass: <input disabled=\"disabled\" type=text name=pass size=30 maxlength=16 value='******' /><br>";
    }*/

  s += "<br>http auth for STA clients: <select name=\"CONFIGauthEnabled\">";
  if (eeprom_data.CONFIGauthEnabled == true)
  {
    s += "<option value =\"true\" selected=\"selected\">Enabled</option>";
    s += "<option value =\"false\">Disabled</option>";
  }
  else
  {
    s += "<option value =\"true\">Enabled</option>";
    s += "<option value =\"false\" selected=\"selected\">Disabled</option>";
  }
  s += "</select>";

  s += "<br>http user: <input type=text name=CONFIGuser size=30 maxlength=8 value='";
  s += eeprom_data.CONFIGuser;

  s += "' />";
  //if (clientConfigAllowed == true)
  //{
  s += "<br>http pass: <input type=text name=CONFIGpass size=30 maxlength=8 value='";
  s += eeprom_data.CONFIGpass;
  s += "' /><br>";
  /*}
    else
    {
    s += "<br>http pass: <input disabled=\"disabled\" type=text name=HTTPpass size=30 maxlength=8 value='******' /><br>";
    }*/

  s += "<br>Reboot after storing <input type=\"checkbox\" name=\"rebootRq\"/>";
  s += "<br>";

  //if (clientConfigAllowed == true)
  s += "<input type='submit' value='Save'></form>";
  //else
  //  s += "<input type='submit' value='Save' disabled=\"disabled\"> changes disabled from external wifi</form>";



  s += "</html>\r\n\r\n";
  server.send(200, "text/html", s);
}
// ----------------------------------- readSettingsESP -----------------------------------
void readSettingsESP()
{
  int i;
  uint32_t datacrc;
  byte eeprom_data_tmp[sizeof(eeprom_data)];

  EEPROM.begin(sizeof(eeprom_data) + sizeof(memcrc));

  for (i = EEPROM_START; i < EEPROM_START+sizeof(eeprom_data); i++)
  {
    eeprom_data_tmp[i] = EEPROM.read(i);
  }

  p_memcrc[0] = EEPROM.read(i++);
  p_memcrc[1] = EEPROM.read(i++);
  p_memcrc[2] = EEPROM.read(i++);
  p_memcrc[3] = EEPROM.read(i++);

  datacrc = crc_byte(eeprom_data_tmp, sizeof(eeprom_data_tmp));

  if (memcrc == datacrc)
  {
    setEEPROM = true;
    memcpy(&eeprom_data, eeprom_data_tmp,  sizeof(eeprom_data));
  }
  else
  {
    eeprom_data.senderEnabled = DEFAULT_SENDER_ENABLED;
    eeprom_data.STAenabled = DEFAULT_STA_ENABLED;
    eeprom_data.disableAP = DEFAULT_DISABLE_AP;
    eeprom_data.CONFIGauthEnabled = DEFAULT_HTTP_AUTH_ENABLED;
    strncpy(eeprom_data.STAssid, STA_SSID_DEFAULT, sizeof(STA_SSID_DEFAULT));
    strncpy(eeprom_data.STApass, STA_PASSWORD_DEFAULT, sizeof(STA_PASSWORD_DEFAULT));
    strncpy(eeprom_data.CONFIGuser, CONFIG_HTTP_USER_DEFAULT, sizeof(CONFIG_HTTP_USER_DEFAULT));
    strncpy(eeprom_data.CONFIGpass, CONFIG_HTTP_PASSWORD_DEFAULT, sizeof(CONFIG_HTTP_PASSWORD_DEFAULT));

  }

}
// ----------------------------------- writeSettingsESP -----------------------------------
void writeSettingsESP()
{
  int i;
  byte eeprom_data_tmp[sizeof(eeprom_data)];

  EEPROM.begin(sizeof(eeprom_data) + sizeof(memcrc));

  memcpy(eeprom_data_tmp, &eeprom_data, sizeof(eeprom_data));

  for (i = EEPROM_START; i < EEPROM_START+sizeof(eeprom_data); i++)
  {
    EEPROM.write(i, eeprom_data_tmp[i]);
  }
  memcrc = crc_byte(eeprom_data_tmp, sizeof(eeprom_data_tmp));

  EEPROM.write(i++, p_memcrc[0]);
  EEPROM.write(i++, p_memcrc[1]);
  EEPROM.write(i++, p_memcrc[2]);
  EEPROM.write(i++, p_memcrc[3]);


  EEPROM.commit();
}

// ----------------------------------- crc_update -----------------------------------
unsigned long crc_update(unsigned long crc, byte data)
{
  byte tbl_idx;
  tbl_idx = crc ^ (data >> (0 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
  tbl_idx = crc ^ (data >> (1 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
  return crc;
}
// ----------------------------------- crc_byte -----------------------------------
unsigned long crc_byte(byte *b, int len)
{
  unsigned long crc = ~0L;
  uint16_t i;

  for (i = 0 ; i < len ; i++)
  {
    crc = crc_update(crc, *b++);
  }
  crc = ~crc;
  return crc;
}
// -----------------------------------  -----------------------------------
// -----------------------------------  -----------------------------------
// -----------------------------------  -----------------------------------

