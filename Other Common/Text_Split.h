#include <stdint.h>

#ifdef __AVR__
#include <Arduino.h>
#else
#include <string>
#endif

#ifndef text_split_h
#define text_split_h

#ifdef __AVR__
void splitText(const uint16_t len, String text, String* sub_text, const int num);
#else
void splitText(const uint16_t len, std::string text, std::string* sub_text, const int num);
#endif

#endif
