#include <Arduino.h>
#include <Vector.h>

#include "AIBus_Handler.h"
#include "Bluetooth_Device.h"
#include "Phone_Text_Handler.h"

#include "Parameter_List.h"

#ifndef bm83_aidf_h
#define bm83_aidf_h

#define BM83_START_BYTE 0xAA

#define BM83_ACK 0x14

#define BM83_STATUS_OFF 0
#define BM83_STATUS_DISCONNECTED 1
#define BM83_STATUS_CONNECTED 2
#define BM83_STATUS_PAIRING 3

#define CALL_STATUS_IDLE 0
#define CALL_STATUS_ACTIVE 1
#define CALL_STATUS_INCOMING 2
#define CALL_STATUS_OUTGOING 3
#define CALL_STATUS_VOICE 4

#define MUSIC_STOP_FF_FR 0x0
#define MUSIC_FF 0x1
#define MUSIC_FF_REPEAT 0x2
#define MUSIC_FR 0x3
#define MUSIC_FR_REPEAT 0x4
#define MUSIC_PLAY 0x5
#define MUSIC_PAUSE 0x6
#define MUSIC_TOGGLE 0x7
#define MUSIC_STOP 0x8
#define MUSIC_NEXT 0x9
#define MUSIC_BACK 0xA

struct BM83Data {
	uint8_t* data;
	uint8_t opcode;
	uint16_t l;

	BM83Data(const uint16_t l, const uint8_t opcode);
	~BM83Data();
	BM83Data(const BM83Data &copy);

	void refreshBMData(uint8_t* data);
};

class BM83 {
public:
	BM83(Stream* serial, AIBusHandler* ai_handler, PhoneTextHandler* text_handler, ParameterList* parameters);
	BM83(Stream* serial, AIBusHandler* ai_handler, PhoneTextHandler* text_handler, ParameterList* parameters, const uint16_t pin_code);
	~BM83();
	void loop();

	void init();

	bool handleAIBus(AIData* ai_msg);
	void sendAIBusHandshake();
private:
	Stream* serial;
	AIBusHandler* ai_handler;
	ParameterList* parameter_list;

	PhoneTextHandler* text_handler;
	BluetoothDevice* active_device = NULL;

	uint16_t pin_code;

	uint8_t bm83_status = BM83_STATUS_OFF, call_status = CALL_STATUS_IDLE;

	String caller_number = "", caller_id = "";

	String song_title = "", artist = "", album = "";
	long song_time = -1;

	bool phone_audio_sel = false, text_control = false;
	
	void sendPowerOn();
	void sendDeviceName(String name);
	void sendDevicePIN();
	void sendDevicePIN(uint16_t pin);

	void activatePairing();

	void sendConnect(BluetoothDevice* device);
	void disconnect();

	void handleBM83Message(BM83Data* msg);
	void handleCallerID(BM83Data* msg);

	void sendBM83Message(BM83Data* msg);
	BM83Data getBM83Message(uint8_t* data, const int l);

	void activateBTDevice(const uint8_t id);

	void sendMusicCommand(const uint8_t command);
};

#endif
