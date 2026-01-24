#include <Arduino.h>
#include <stdint.h>
#include <elapsedMillis.h>
#include <Vector.h>

#include "AIBus.h"

#ifndef aibus_handler_h
#define aibus_handler_h

#define AI_DELAY_U 20
#define AI_DELAY_M 2
#define L_WAIT 20
#define AI_WAIT 5
#define RX_TIMER 200

#define REPEAT_DELAY 100
#define MAX_REPEAT 16

#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFF
#endif

#ifndef AI_CACHE_SIZE
#define AI_CACHE_SIZE 16
#endif

#define AIDATA_LIMIT (0x30 - 4)

enum aibus_read_result_t : int8_t {
	AIBUS_READ_OK_SERIAL,
	AIBUS_READ_OK_CACHED,
	AIBUS_READ_OK_MULTIPLE,
	AIBUS_READ_NODATA,
	AIBUS_READ_INCOMPLETE,
	AIBUS_READ_TIMEOUT,
	AIBUS_READ_INVALID_CHECKSUM,
	AIBUS_READ_MULTIPLE_TIMEOUT,
	AIBUS_READ_INVALID_MULTIPLE,
};

class AIBusHandler {
public:
	AIBusHandler(Stream* serial, const int8_t rx_pin, const uint8_t id, const unsigned int ai_cache_size = AI_CACHE_SIZE);
	~AIBusHandler();

	virtual int dataAvailable();
	virtual int dataAvailable(const bool cache);

	virtual bool readAIData(AIData* ai_d);
	virtual bool readAIData(AIData* ai_d, const bool cache, const bool multiple = true);

	virtual aibus_read_result_t readAIDataErr(AIData* ai_d);
	virtual aibus_read_result_t readAIDataErr(AIData* ai_d, const bool cache, const bool multiple = true);

	virtual bool readAIData(AIData* ai_d, uint8_t* data, const uint8_t d_l);

	virtual bool writeAIData(AIData* ai_d);
	virtual bool writeAIData(AIData* ai_d, const bool acknowledge);
	virtual void sendAcknowledgement(const uint8_t sender, const uint8_t receiver);
	
	virtual void cacheMessage(AIData* ai_msg);
	virtual bool cachePending(const uint8_t id);

protected:
	Stream* ai_serial;
	int8_t rx_pin = -1;

	AIData* cached_byte;
	Vector<AIData> cached_vec;
	
	uint8_t id;

	virtual bool awaitAcknowledgement(AIData* ai_d);
	virtual bool getID(const uint8_t id);

	inline void clearSerial();
};

bool getInitMessage(AIData* ai_d);
bool getPoweroffMessage(AIData* ai_d);

bool getPositiveResult(const aibus_read_result_t result);

#endif
