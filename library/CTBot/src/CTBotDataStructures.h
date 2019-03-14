#pragma once
#include <Arduino.h>

enum CTBotMessageType {
	CTBotMessageNoData   = 0,
	CTBotMessageText     = 1,
	CTBotMessageQuery    = 2,
	CTBotMessageLocation = 3
};

typedef struct TBUser {
	int32_t  id;
	bool     isBot;
	String   firstName;
	String   lastName;
	String   username;
	String   languageCode;
};

typedef struct TBLocation{
	float longitude;
	float latitude;
};

typedef struct TBMessage {
	int32_t          messageID;
	TBUser           sender;
	int32_t          date;
	String           text;
	String           chatInstance;
	String           callbackQueryData;
	String           callbackQueryID;
	TBLocation       location;
	CTBotMessageType messageType;
};

