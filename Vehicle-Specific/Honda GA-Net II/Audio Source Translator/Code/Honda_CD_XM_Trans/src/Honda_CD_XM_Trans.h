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

#if defined(DXCORE) || defined(MEGACOREX)
#define HW_IEBUS
#endif

// #define AI_DEBUG

#if defined(HAVE_HWSERIAL1) && !defined(AI_DEBUG) && !defined(DXCORE) && !defined(MEGACOREX)
#define AISerial Serial1
#else
#define AISerial Serial
#endif

#ifndef AI_DEBUG
// #define AI_DEBUG
#endif

#ifdef HW_IEBUS
#define AI_RX PIN_PA7

#define GA_ON PIN_PD1
#define ILL_ANODE PIN_PD2

#define IEBUS_ACTIVE PIN_PC2

#define ILL_CS PIN_PD4
#define AUDIO_ON PIN_PD3
#define MAIN_POWER PIN_PD5

#define TRUNK_OPEN PIN_PD0
#define SPDIF_RESET PIN_PD6

#define IESerial Serial1
#else
#define AI_RX 2

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
#endif

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
	#ifdef HW_IEBUS
	EnIEBusHandler ie_handler = EnIEBusHandler(&IESerial, IEBUS_ACTIVE);
	#else
	EnIEBusHandler ie_handler = EnIEBusHandler(IEBUS_RX, IEBUS_TX);
	#endif

	HondaIMIDHandler imid_handler = HondaIMIDHandler(&ie_handler, &ai_handler, &parameters);
	HondaTapeHandler tape_handler = HondaTapeHandler(&ie_handler, &ai_handler, &parameters, &imid_handler);
	HondaXMHandler xm_handler = HondaXMHandler(&ie_handler, &ai_handler, &parameters, &imid_handler);
	HondaCDHandler cd_handler = HondaCDHandler(&ie_handler, &ai_handler, &parameters, &imid_handler);

	BrightnessHandler brightness_handler = BrightnessHandler(ILL_CS, ILL_ANODE);

	elapsedMillis function_timer, screen_request_timer, ping_timer, power_ping_timer, dimension_request_timer, memory_timer;

	elapsedMillis door_timer;
	bool door_timer_enabled = false;

	bool keyed_on = false; //True if the key has been on at any time during this power cycle.
	
	void interpretIEData(IE_Message ie_msg);
	void sendIMIDRequest();
	void powerOff();
};

void setup();
void loop();

int freeMemory();

#endif
