#include "AIBus_Serial.h"

#ifdef __cplusplus
extern "C" {
#endif
int aiserialOpen(const char* port) {
	int serial = open(port, O_RDWR);
	
	struct termios tty;
	tcgetattr(serial, &tty);

	tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSTOPB;

	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;

	tty.c_cflag |= CREAD | CLOCAL;

	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO;
	tty.c_lflag &= ~ECHOE;
	tty.c_lflag &= ~ECHONL;
	tty.c_lflag &= ~ISIG;
	tty.c_iflag &= ~(IXON |IXOFF |IXANY);
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

	tty.c_oflag &= ~OPOST;
	tty.c_oflag &= ~ONLCR;
	tty.c_cc[VTIME] = 5;

	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);
	tcsetattr(serial, TCSANOW, &tty);

	return serial;
}

void aiserialClose(int port) {
	close(port);
}

int aiserialRead(int port, char* buffer, int l) {
	return read(port, buffer, l);
}

void aiserialWrite(int port, char* buffer, int l) {
	write(port, buffer, l);
}

char aiserialReadByte(int port) {
	char bytes[1];
	aiserialRead(port, bytes, 1);
	return bytes[0];
}

void aiserialWriteByte(int port, char byte) {
	char bytes[1] = {byte};
	aiserialWrite(port, bytes, 1);
}

int aiserialBytesAvailable(int port) {
	int bytes = 0;
	ioctl(port, FIONREAD, &bytes);
	return bytes;
}

#ifdef __cplusplus
}
#endif