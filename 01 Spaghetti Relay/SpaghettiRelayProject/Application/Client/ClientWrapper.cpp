#include "ClientWrapper.h"
#include "Client.h"

Client* client = new Client();

int init(uint16_t port, char* address)
{
	if (startup())
		return STARTUP_ERROR;

	return client->init(port, address);
	stop();
}

int readMessage(char* buffer, int32_t size)
{
	return client->readMessage(buffer, size);
}

int sendMessage(char* data, int32_t length)
{
	return client->sendMessage(data, length);
}

void stop()
{
	client->stop();
	shutdown();
}
