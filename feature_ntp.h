#pragma once

// NTP
#ifdef NTP_TIME_ENABLE
#include <WiFiUdp.h>

WiFiUDP Udp;
int ntpRefreshDelay = 180;
uint32_t lastTimeRefersh = 0;
uint32_t timeRefershPeriod = 24 * 60 * 60 * 1000; //once a day
bool NTPenable = false;

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address)
{
	//buffer to hold incoming & outgoing packets
	uint8_t packetBuffer[NTP_PACKET_SIZE];
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011; // LI, Version, Mode
	packetBuffer[1] = 0;		  // Stratum, or type of clock
	packetBuffer[2] = 6;		  // Polling Interval
	packetBuffer[3] = 0xEC;		  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;
	// all NTP fields have been given values, now you can send a packet requesting a timestamp:
	Udp.beginPacket(address, 123); //NTP requests are to port 123
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
}

time_t getNtpTime()
{
	if (!NTPenable || WiFi.status() != WL_CONNECTED)
		return 0;

	while (Udp.parsePacket() > 0)
		yield(); // discard any previously received packets

	IPAddress timeServerIP(129, 6, 15, 28); //129.6.15.28 = time-a.nist.gov

	String ntpServer = readConfigString(NTP_SERVER_addr, NTP_SERVER_size);
	const int timeZone = readConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size).toInt();

	if (!timeServerIP.fromString(ntpServer))
	{
		if (WiFi.hostByName(ntpServer.c_str(), timeServerIP) != 1)
		{
			timeServerIP = IPAddress(129, 6, 15, 28);
		}
	}

	sendNTPpacket(timeServerIP);
	const uint32_t beginWait = millis();
	while (millis() - beginWait < 1500)
	{
		const int size = Udp.parsePacket();
		if (size >= NTP_PACKET_SIZE)
		{
			uint8_t packetBuffer[NTP_PACKET_SIZE];	 //buffer to hold incoming & outgoing packets
			Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
			timeIsSet = true;
			// convert four bytes starting at location 40 to a long integer
			uint32_t secsSince1900 = (uint32_t)packetBuffer[40] << 24;
			secsSince1900 |= (uint32_t)packetBuffer[41] << 16;
			secsSince1900 |= (uint32_t)packetBuffer[42] << 8;
			secsSince1900 |= (uint32_t)packetBuffer[43];
			//NTPenable = false;
			setSyncProvider(nullptr);
			Udp.stop();
			lastTimeRefersh = millis();
			return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
		}
		yield();
	}
	return 0; // return 0 if unable to get the time
}

void restartNTP()
{
	if (readConfigString(NTP_ENABLE_addr, NTP_ENABLE_size) == SWITCH_ON_NUMBER)
		NTPenable = true;

	if (NTPenable)
	{
		ntpRefreshDelay = readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size).toInt();
		Udp.begin(UDP_LOCAL_PORT);
		setSyncProvider(getNtpTime);
		setSyncInterval(ntpRefreshDelay); //60 sec. = 1 min.
		timeRefershPeriod = readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size).toInt() * 1000;
	}
}

bool tryHandleNtpCommand(commandTokens& cmd, String& str, String& eq, uint8_t channel, bool isAdmin)
{
	if (cmd.command == CMD_NTP_SERVER)
	{
		str += REPLY_NTP_SERVER;
		str += eq;
		str += cmd.arguments[0];
		str += QUOTE;
		writeConfigString(NTP_SERVER_addr, NTP_SERVER_size, cmd.arguments[0]);
	}
	else if (cmd.command == CMD_NTP_TIME_ZONE)
	{
		int timeZone = cmd.arguments[0].toInt();
		str += REPLY_NTP_TIME_ZONE;
		str += eq;
		str += String(timeZone);
		str += QUOTE;
		writeConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size, String(timeZone));
	}
	else if (cmd.command == CMD_NTP_REFRESH_DELAY)
	{
		ntpRefreshDelay = cmd.arguments[0].toInt();
		str += REPLY_NTP_REFRESH_DELAY;
		str += eq;
		str += String(ntpRefreshDelay);
		str += QUOTE;
		writeConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size, String(ntpRefreshDelay));
		setSyncInterval(ntpRefreshDelay);
	}
	else if (cmd.command == CMD_NTP_REFRESH_PERIOD)
	{
		timeRefershPeriod = cmd.arguments[0].toInt();
		str += REPLY_NTP_REFRESH_PERIOD;
		str += eq;
		str += String(timeRefershPeriod);
		str += QUOTE;
		writeConfigString(NTP_REFRESH_PERIOD_addr, NTP_REFRESH_PERIOD_size, String(timeRefershPeriod));
	}
	else if (cmd.command == CMD_NTP_ENABLE)
	{
		if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
		{
			writeConfigString(NTP_ENABLE_addr, NTP_ENABLE_size, SWITCH_OFF_NUMBER);
			NTPenable = false;
			setSyncProvider(nullptr);
			Udp.stop();
			str += REPLY_NTP_ENABLE;
			str += eq;
			str += SWITCH_OFF;
		}
		else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
		{
			writeConfigString(NTP_ENABLE_addr, NTP_ENABLE_size, SWITCH_ON_NUMBER);
			NTPenable = true;
			Udp.begin(UDP_LOCAL_PORT);
			setSyncProvider(getNtpTime);
			setSyncInterval(ntpRefreshDelay);
			str += REPLY_NTP_ENABLE;
			str += eq;
			str += SWITCH_ON;
		}
		else
		{
			str += REPLY_INCORRECT_VALUE;
			str += cmd.arguments[0];
		}
	}
	else
	{
		return false;
	}
	return true;
}
#endif
