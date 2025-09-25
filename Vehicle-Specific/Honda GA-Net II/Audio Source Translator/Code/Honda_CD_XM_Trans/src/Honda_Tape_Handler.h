#include "En_IEBus_Handler.h"
#include "AIBus_Handler.h"
#include "Handshakes.h"
#include "Parameter_List.h"
#include "Honda_Source_Handler.h"
#include "Honda_IMID_Handler.h"

#include "Trans_EEPROM.h"

#include <stdint.h>
#include <Arduino.h>

#ifndef honda_tape_handler_h
#define honda_tape_handler_h

#define TAPE_MODE_PLAY 2
#define TAPE_MODE_REW 5
#define TAPE_MODE_FF 4
#define TAPE_MODE_REVSKIP 7
#define TAPE_MODE_FWDSKIP 6
#define TAPE_MODE_LOAD 0x10
#define TAPE_MODE_EJECT 0x11
#define TAPE_MODE_IDLE 0

#define HONDA_BUTTON_DIR 0x20
#define HONDA_BUTTON_FF 0x21
#define HONDA_BUTTON_REW 0x22
#define HONDA_BUTTON_SKIPFWD 0x23
#define HONDA_BUTTON_SKIPREV 0x24
#define HONDA_BUTTON_REPEAT 0x25
#define HONDA_BUTTON_NR 0x29

class HondaTapeHandler : public HondaSourceHandler {
public:
	HondaTapeHandler(EnIEBusHandler* ie_driver, AIBusHandler* ai_driver, ParameterList* parameter_list, HondaIMIDHandler* imid_handler);

	void loop();
	
	void interpretTapeMessage(IE_Message* the_message);
	void readAIBusMessage(AIData* the_message);
	
	void sendSourceNameMessage();
	
private:
	bool repeat_on = false, nr_on = false, fwd = true, cro2 = false;
	uint8_t tape_mode = 0, track_count = 0;

	bool autostart = false, fwd_start = false;
	bool change_dir = false; //Change direction at next play message.

	bool setting_changed = false;

	bool nr_timer_enabled = false;
	elapsedMillis nr_timer;

	int8_t desired_track_count = 0;

	HondaIMIDHandler* imid_handler;
	
	uint8_t getDirectionByte(const bool rev, const bool repeat, const bool nr, const bool cr_o2);

	bool getDisplaySymbol();

	void sendSourceNameMessage(const uint8_t id);
	
	void sendButtonMessage(const uint8_t button);
	void sendButtonMessage(const uint8_t button, const uint8_t tracks);

	void sendStatusRequest();

	void sendTapeUpdateMessage(const uint8_t receiver);
	void sendTapeTextMessage();
	void sendFunctionTextMessage();

	void createTapeMenu();
	void createTapeMenuOption(const uint8_t option);
};

#endif
