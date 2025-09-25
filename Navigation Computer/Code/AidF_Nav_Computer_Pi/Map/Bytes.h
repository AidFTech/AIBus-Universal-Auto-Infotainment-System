#include <stdint.h>

#ifndef bytes_h
#define bytes_h

struct Bytes {
	uint8_t* data;
	int l;

	Bytes(const int l);
	Bytes(const int l, uint8_t* data);
	Bytes(const Bytes &copy);
	~Bytes();
	uint8_t& operator[](int i);
	Bytes& operator=(const Bytes &copy);
};

#endif