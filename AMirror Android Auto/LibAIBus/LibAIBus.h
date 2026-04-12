#include "Client_AIBus_Handler.h"

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include <cstdlib>

#ifndef libaibus_h
#define libaibus_h

#ifdef __cplusplus
extern "C" {
#endif

struct ElapsedMillis {
	unsigned long time = 0;
	bool* run;
};

ClientHandlerParameters* getClientHandler(ElapsedMillis* timer, const char* path, const uint8_t id);
void freeClientHandler(ClientHandlerParameters* client_handler);

bool readAIData(ClientAIBusHandler* aibus_handler, AIData* ai_d);

ElapsedMillis* getElapsedMillis(bool* run);
void freeElapsedMillis(ElapsedMillis* timer);

void startTimer(pthread_t timer_thread, ElapsedMillis* timer);
void startAIBus(pthread_t aibus_thread, ClientHandlerParameters* client_handler);
void stopThread(pthread_t thread);

void *millisThread(void* millis_v);

#ifdef __cplusplus
}
#endif

#endif