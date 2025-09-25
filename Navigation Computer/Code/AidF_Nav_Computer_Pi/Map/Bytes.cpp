#include "Bytes.h"

Bytes::Bytes(const int l) {
	this->data = new uint8_t[l];
	this->l = l;
}

Bytes::Bytes(const int l, uint8_t* data) : Bytes(l) {
	for(int i=0;i<l;i+=1)
		this->data[i] = data[i];
}

Bytes::Bytes(const Bytes &copy) {
	this->l = copy.l;
	this->data = new uint8_t[this->l];

	for(int i=0;i<this->l;i+=1)
		this->data[i] = copy.data[i];
}

Bytes::~Bytes() {
	delete[] this->data;
}

uint8_t& Bytes::operator[](int i) {
	return this->data[i];
}

Bytes& Bytes::operator=(const Bytes& copy) {
	delete[] this->data;
	this->l = copy.l;
	this->data = new uint8_t[this->l];

	for(int i=0;i<this->l;i+=1)
		this->data[i] = copy.data[i];

	return *this;
}
