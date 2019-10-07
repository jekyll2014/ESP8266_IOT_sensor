#define DEFAULT_STA_ENABLED true               // режим клиента (station) 
#define DEFAULT_DISABLE_AP true                // выключать режим точки доступа (AP) при подключенном режиме клиента

#define NAME_TAG "ESP"

#define AP_SSID NAME_TAG "8266-"
#define AP_PASSWORD_PREFIX "SECRET"

bool rebootReq = false;

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

void setupNEW() 
{
	WiFi.disconnect(true);
	WiFi.persistent(false);
	WiFi.setPhyMode(WIFI_PHY_MODE_11G);
	delay(500);
	WiFi.onEvent(WiFiEvent);



	if (STAenabled == true)
	{
		Serial.print("STA enabled, checking : ");  Serial.println(STAssid);

		if (checkAPinair(String(STAssid)))
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
}

void loopNEW()
{
	processAPcheck();
}

void WiFiEvent(WiFiEvent_t event) {
	//Serial.printf("[WiFi-event] event: %d\n", event);

	switch (event) {
	case WIFI_EVENT_STAMODE_GOT_IP:
		staConnectTime = millis();
		if (staInitOk == false)
		{
			staInitOk = true;
			nextSenderTick = millis() + 30000;

			ipSTA = WiFi.localIP();
			ipSTAstr = String(ipSTA[0]) + '.' + String(ipSTA[1]) + '.' + String(ipSTA[2]) + '.' + String(ipSTA[3]);

			if (disableAP == true)
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
			if (disableAP == true)
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

boolean checkAPinair(String name) {
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

void processAPcheck() 
{
	if (STAenabled == true && staInitOk == false)
	{
		if (nextapCheckTick <= millis())
		{
			nextapCheckTick = millis() + apCheckTickTime;

			Serial.print("STA not connected, checking ssid : ");  Serial.println(STAssid);

			if (checkAPinair(String(STAssid)))
			{
				WiFi.mode(WIFI_AP_STA);

				//WiFi.softAP(AP_NameChar, AP_PassChar);
				WiFi.begin(STAssid, STApass);
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

void STAinit()
{

	WiFi.begin(STAssid, STApass);


	uint8_t mac[WL_MAC_ADDR_LENGTH];
	WiFi.macAddress(mac);



	STAmacStr = String(mac[WL_MAC_ADDR_LENGTH - 6], HEX) + ":" +
		String(mac[WL_MAC_ADDR_LENGTH - 5], HEX) + ":" +
		String(mac[WL_MAC_ADDR_LENGTH - 4], HEX) + ":" +
		String(mac[WL_MAC_ADDR_LENGTH - 3], HEX) + ":" +
		String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + ":" +
		String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);



	Serial.print("STA SSID: ");
	Serial.println(STAssid);


}

void softAPinit()
{

	//Serial.println();

	uint8_t mac[WL_MAC_ADDR_LENGTH];
	WiFi.softAPmacAddress(mac);

	String AP_NameString = AP_SSID;

	//char AP_NameChar[AP_NameString.length() + 1];
	//char AP_PassChar[AP_PassString.length() + 1];

	for (int i = 0; i < AP_NameString.length(); i++)
		AP_NameChar[i] = AP_NameString.charAt(i);
	AP_NameChar[AP_NameString.length()] = 0;


	String AP_PassString = AP_PASSWORD_PREFIX;

	for (int i = 0; i < AP_PassString.length(); i++)
		AP_PassChar[i] = AP_PassString.charAt(i);
	AP_PassChar[AP_PassString.length()] = 0;

	WiFi.softAP(AP_NameChar, AP_PassChar);
	Serial.print("AP SSID: ");  Serial.println(AP_NameChar);
	//Serial.print("AP pass: ");  Serial.println(AP_PassChar);

	ipAP = WiFi.softAPIP();

	ipAPstr = String(ipAP[0]) + '.' + String(ipAP[1]) + '.' + String(ipAP[2]) + '.' + String(ipAP[3]);
	checkClientNetStr = String(ipAP[0]) + '.' + String(ipAP[1]) + '.' + String(ipAP[2]) + '.';

	Serial.print("AP IP address: ");
	Serial.println(ipAPstr);

	Serial.print("AP MAC address: ");
	Serial.println(APmacStr);


}
