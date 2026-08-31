#include <Arduino.h>
#include <elapsedMillis.h>

#include "AIBus.h"
#include "En_IEBus_Handler.h"

#include "Handshakes.h"
#include "Honda_CD_Handler.h"
#include "Honda_Tape_Handler.h"
#include "Honda_XM_Handler.h"
#include "Honda_IMID_Handler.h"
#include "Brightness_Handler.h"

#include "Parameter_List.h"


#ifndef honda_cd_xm_trans_h
#define honda_cd_xm_trans_h

// #define AI_DEBUG

#ifndef AI_RX
#define AI_RX 2
#endif

#if defined(HAVE_HWSERIAL1) && !defined(AI_DEBUG)
#define AISerial Serial1
#else
#define AISerial Serial
#endif

#ifndef AI_DEBUG
// #define AI_DEBUG
#endif

#define GA_ON 4
#define ILL_ANODE 5

#define IEBUS_TX 6
#define IEBUS_RX 7

#define AIBUS_BLOCK 8

#define ILL_CS 9
// #define MEMORY_CHECK

#define MAIN_POWER 14
#define REC_SET 15
#define REC_CLEAR 16

#define AUDIO_ON 17

#define TRUNK_OPEN 20

#define GAH_READ_H &PINA
#define GAH_READ_L &PINC

#define GAH_COUNT_0 10
#define GAH_COUNT_1 11
#define GAH_COUNT_2 12
#define GAH_COUNT_3 13

#define GAH_COUNT_ENABLE 41
#define GAH_COUNT_CLEAR 40

#define SPDIF_RESET A8

#define FUNCTION_DELAY 4500
#define SOURCE_DELAY 5000

#define POWER_PING_TIMER 45250

#define DIMENSION_REQUEST_TIMER 4000

#define DOOR_TIMER 30000

#define CACHE_SIZE 32

// #define IE_DEBUG

class HondaCDXMTrans {
public:
	void setup();
	void loop();
private:
	ParameterList parameters;

	EnAIBusHandler ai_handler = EnAIBusHandler(&AISerial, AI_RX, 8, EN_AI_CACHE_SIZE, 0);
	EnIEBusHandler ie_handler = EnIEBusHandler(IEBUS_RX, IEBUS_TX);

	HondaIMIDHandler imid_handler = HondaIMIDHandler(&ie_handler, &ai_handler, &parameters);
	HondaTapeHandler tape_handler = HondaTapeHandler(&ie_handler, &ai_handler, &parameters, &imid_handler);
	HondaXMHandler xm_handler = HondaXMHandler(&ie_handler, &ai_handler, &parameters, &imid_handler);
	HondaCDHandler cd_handler = HondaCDHandler(&ie_handler, &ai_handler, &parameters, &imid_handler);

	BrightnessHandler brightness_handler = BrightnessHandler(ILL_CS, ILL_ANODE);

	elapsedMillis function_timer, screen_request_timer, ping_timer, power_ping_timer, dimension_request_timer, memory_timer;

	elapsedMillis door_timer;
	bool door_timer_enabled = false;
	
	void interpretIEData(IE_Message ie_msg);
	void sendIMIDRequest();
	void powerOff();
};

void setup();
void loop();

int freeMemory();

#endif
