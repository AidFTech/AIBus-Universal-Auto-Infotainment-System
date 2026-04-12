#include "LibAIBus.h"

//Get a client parameter object with the AIBus handler.
ClientHandlerParameters* getClientHandler(ElapsedMillis* timer, const char* path, const uint8_t id) {
	ClientHandlerParameters* client_params = new ClientHandlerParameters;
	client_params->socket_path = path;
	client_params->running = timer->run;
	client_params->ai_handler = new ClientAIBusHandler(path, id);

	client_params->ai_handler->setTimer(&timer->time);

	return client_params;
}

//Free up the client parameter object.
void freeClientHandler(ClientHandlerParameters* client_handler) {
	delete client_handler->ai_handler;
	delete client_handler;
}

//Get whether there is AIBus data pending.
bool readAIData(ClientAIBusHandler* aibus_handler, AIData* ai_d) {
	return aibus_handler->readAIData(ai_d);
}

//Get a timer object.
ElapsedMillis* getElapsedMillis(bool* run) {
	ElapsedMillis* timer = new ElapsedMillis;
	timer->run = run;

	return timer;
}

//Free a timer object.
void freeElapsedMillis(ElapsedMillis* timer) {
	delete timer;
}

//Start the timer.
void startTimer(pthread_t timer_thread, ElapsedMillis* timer) {
	pthread_create(&timer_thread, NULL, millisThread, (void*)timer);
}

//Start the AIBus thread.
void startAIBus(pthread_t aibus_thread, ClientHandlerParameters* client_handler) {
	pthread_create(&aibus_thread, NULL, socketThread, (void*)client_handler);
}

//Stop a thread.
void stopThread(pthread_t thread) {
	pthread_join(thread, NULL);
}

//Timer thread function.
void *millisThread(void* millis_v) {
	ElapsedMillis* elapsed_millis = (ElapsedMillis*)millis_v;

	while(*elapsed_millis->run) {
		usleep(1000);

		elapsed_millis->time += 1;

		if(!*elapsed_millis->run)
			break;
	}

	void* result;
	return result;
}