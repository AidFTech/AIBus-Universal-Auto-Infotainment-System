#include "Text_Split.h"

//Split text by spaces.
#ifdef __AVR__
void splitText(const uint16_t len, String text, String* sub_text, const int num) {
#else
void splitText(const uint16_t len, std::string text, std::string* sub_text, const int num) {
#endif
	if(text.length() > 0) {
		int text_end = text.length();
		for(int i=text.length()-1; i >= 0; i-= 1) {
			#ifdef __AVR__
			if(text.charAt(i) > ' ')
			#else
			if(text.at(i) > ' ')
			#endif
				break;
			else
				text_end = i;
		}
		#ifdef __AVR__
		text = text.substring(0, text_end);
		#else
		text = text.substr(0, text_end);
		#endif

		for(int i=0;i<text.length();i+=1) {
			#ifdef __AVR__
			if(text.charAt(i) < 0x20 && text.charAt(i) != 0) {
			#else
			if(text.at(i) < 0x20 && text.at(i) != 0) {
			#endif
				#ifdef __AVR__
				text.remove(i);
				#else
				text.erase(i, i+1);
				#endif
				i -= 1;
			}
		}

		text += " ";
	}

	for(int i=0;i<num;i+=1) {
		if(text.length() > 0) {
			int last_space = -1, space = -1;

			#ifdef __AVR__
			if(text.charAt(0) == ' ')
				text.remove(0);
			#else
			if(text.at(0) == ' ')
				text.erase(0, 1);
			#endif
			
			do {
				last_space = space;
				#ifdef __AVR__
				space = text.indexOf(' ', last_space + 1);
				#else
				space = text.find(' ', last_space + 1);
				#endif
				
				if(space >= len || space < 0 || space >= text.length() - 1)
					break;
			} while(space >= 0);

			if(space >= text.length() - 1 && i >= num - 1)
				#ifdef __AVR__
				text.remove(text.length() - 1);
				#else
				text.erase(text.length() - 1);
				#endif
			
			if(last_space >= 0 && last_space < len && space > len) {
				#ifdef __AVR__
				sub_text[i] = text.substring(0, last_space);
				text = text.substring(last_space + 1, text.length());
				#else
				sub_text[i] = text.substr(0, last_space);
				text = text.substr(last_space + 1, text.length() - (last_space + 1));
				#endif
			} else if(space == len) {
				#ifdef __AVR__
				sub_text[i] = text.substring(0,len);
				text = text.substring(len + 1,text.length());
				#else
				sub_text[i] = text.substr(0,len);
				text = text.substr(len + 1,text.length() - (len + 1));
				#endif
			} else {
				if(text.length() >= len) {
					#ifdef __AVR__
					sub_text[i] = text.substring(0,len);
					text = text.substring(len,text.length());
					#else
					sub_text[i] = text.substr(0,len);
					text = text.substr(len,text.length() - len);
					#endif
				} else {
					sub_text[i] = text;
					text = "";
				}
			}
		}
	}
}