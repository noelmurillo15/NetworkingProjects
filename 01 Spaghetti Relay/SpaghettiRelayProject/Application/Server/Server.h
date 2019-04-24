#ifndef _SERVER_H_
#define _SERVER_H_

#include "../platform.h"
#include "../definitions.h"


class Server {

	bool m_shutdown;
	SOCKET server;
	SOCKET client;

public:
	Server();
	~Server();

	int init(uint16_t port);
	void stop();

	int readMessage(char* buffer, int32_t size);
	int sendMessage(char* data, int32_t length);
};
#endif