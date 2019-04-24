#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "../platform.h"
#include "../definitions.h"


class Client {
	SOCKET sock;
	bool m_shutdown;

public:
	Client();
	~Client();

	int init(uint16_t port, char* address);
	void stop();

	int readMessage(char* buffer, int32_t size);
	int sendMessage(char* data, int32_t length);
};
#endif