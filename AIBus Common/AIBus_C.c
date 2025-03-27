#include "AIBus_C.h"

//Create an AIBus message from a length, sender, and receiver.
AIData* createMessage(const uint16_t l, const uint8_t sender, const uint8_t receiver) {
	AIData* ai_msg;
	ai_msg = (AIData*) malloc(sizeof(AIData));

	ai_msg->l = l;
	ai_msg->sender = sender;
	ai_msg->receiver = receiver;

	ai_msg->data = (uint8_t*) malloc(l);
}

//Free AIData memory.
void clearMessage(AIData* ai_msg) {
	free(ai_msg->data);
	free(ai_msg);
}

//Populate an AIBus message with data.
void refreshAIData(AIData* ai_msg, uint8_t* data) {
	const uint16_t l = ai_msg->l;

	for(int i=0;i<l;i+=1)
		ai_msg->data[i] = data[i];
}

//Get the checksum of a message.
uint8_t getChecksum(AIData* ai_msg) {
	uint8_t chex = ai_msg->sender ^ ai_msg->receiver ^ (ai_msg->l + 2);

	for(int i=0;i<ai_msg->l;i+=1)
		chex ^= ai_msg->data[i];

	return chex;
}

//Get the bytes to be sent over UART.
void getBytes(AIData* ai_msg, uint8_t* b) {
	b[0] = ai_msg->sender;
	b[1] = (ai_msg->l + 2)&0xFF;
	b[2] = ai_msg->receiver;

	for(int i=0;i<ai_msg->l;i+=1)
		b[i+3] = ai_msg->data[i];

	b[ai_msg->l + 3] = getChecksum(ai_msg);
}

//Create an acknowledgment message. **All AIBus devices must acknowledge messages sent directly to them with this message.**
AIData* createAckMessage(const uint8_t sender, const uint8_t receiver) {
	AIData* ack_msg = createMessage(1, sender, receiver);
	ack_msg->data[0] = 0x80;

	return ack_msg;
}