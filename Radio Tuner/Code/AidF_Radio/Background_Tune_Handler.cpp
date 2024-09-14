#include "Background_Tune_Handler.h"

BackgroundTuneHandler::BackgroundTuneHandler(Si4735Controller* br_tuner, ParameterList* parameter_list) {
	this->br_tuner = br_tuner;
	this->parameter_list = parameter_list;
}