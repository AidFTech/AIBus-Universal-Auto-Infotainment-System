#ifndef aibus_serial_h
#define aibus_serial_h

#ifdef __cplusplus
extern "C" {
#endif

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

int aiserialOpen(const char* port);
void aiserialClose(int port);

int aiserialRead(int port, char* buffer, int l);
void aiserialWrite(int port, char* buffer, int l);
char aiserialReadByte(int port);
void aiserialWriteByte(int port, char byte);

int aiserialBytesAvailable(int port);

#ifdef __cplusplus
}
#endif
#endif
