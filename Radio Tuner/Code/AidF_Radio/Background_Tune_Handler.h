#include <stdint.h>
#include <Arduino.h>
#include <Vector.h>

#include "Si4735_AidF.h"
#include "Parameter_List.h"

#ifndef background_tune_handler_h
#define background_tune_handler_h

class BackgroundTuneHandler {
	public:
		BackgroundTuneHandler(Si4735Controller* br_tuner, ParameterList* parameter_list);
	private:
		Si4735Controller* br_tuner;
		ParameterList* parameter_list;
		
		
};

#endif