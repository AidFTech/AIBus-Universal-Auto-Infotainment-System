#include <pthread.h>
#include <stdint.h>

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>

#include "Ini_Context.h"
#include "Serial_AIBus_Handler.h"

using namespace std;

#ifndef nav_edid_check_h
#define nav_edid_check_h

#define FILE_PATH "./AidF_Screen_Info.ini"

struct ElapsedMillis {
	unsigned long time = 0;
	bool* run;
};

int main(int argc, char* args[]);

void *millisThread(void* millis_v);

#endif