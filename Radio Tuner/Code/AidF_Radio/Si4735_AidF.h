#include <stdint.h>
#include <Si4735.h>

#include "AIBus_Handler.h"
#include "Parameter_List.h"
#include "Text_Handler.h"

#ifndef si4735_aidf_h
#define si4735_aidf_h

class Si4735Controller {
public:
	Si4735Controller(AIBusHandler* ai_handler, ParameterList* parameters, TextHandler* text_handler);
};

#endif
