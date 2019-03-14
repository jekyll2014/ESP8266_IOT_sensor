#pragma once

#ifndef CTBOT_INLINE_KEYBOARD
#define CTBOT_INLINE_KEYBOARD

#include <Arduino.h>
#include "ArduinoJson.h"


enum CTBotInlineKeyboardButtonType {
	CTBotKeyboardButtonURL    = 1,
	CTBotKeyboardButtonQuery  = 2
};

class CTBotInlineKeyboard
{
private:
	DynamicJsonBuffer m_jsonBuffer;
	JsonObject *m_root;
	JsonArray  *m_rows;
	JsonArray  *m_buttons;
	bool m_isRowEmpty;

	void initialize(void);

public:
	CTBotInlineKeyboard();
	~CTBotInlineKeyboard();

	// flush the buffer and initialize the data structure
	void flushData(void);

	// add a new empty row of buttons
	// return:
	//    true if no error occurred
	bool addRow(void);

	// add a button in the current row
	// params:
	//   text   : the text displayed as button label
	//   command: URL (if buttonType is CTBotKeyboardButtonURL)
	//            callback query data (if buttonType is CTBotKeyboardButtonQuery)
	// return:
	//    true if no error occurred
	bool addButton(String text, String command, CTBotInlineKeyboardButtonType buttonType);

	// generate a string that contains the inline keyboard formatted in a JSON structure. 
	// Useful for CTBot::sendMessage()
	// returns:
	//   the JSON of the inline keyboard 
	String getJSON(void);
};



#endif
