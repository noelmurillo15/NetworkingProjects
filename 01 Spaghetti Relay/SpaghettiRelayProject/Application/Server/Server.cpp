#include "Server.h"


Server::Server() {

}

Server::~Server() {

}

int Server::init(uint16_t port) {
	m_shutdown = false;
	server = INVALID_SOCKET;
	client = INVALID_SOCKET;

	sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.S_un.S_addr = INADDR_ANY;

	//	Create Socket
	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server == INVALID_SOCKET)
		return SETUP_ERROR;

	//	Bind
	int result = bind(server, (SOCKADDR*)&saddr, sizeof(saddr));
	if (result == SOCKET_ERROR)
		return BIND_ERROR;

	//	Listen for client
	if (listen(server, SOMAXCONN) == SOCKET_ERROR)
		return SETUP_ERROR;

	//	Accept
	client = accept(server, NULL, NULL);
	if (client == INVALID_SOCKET) {
		if (m_shutdown)
			return SHUTDOWN;
		else
			return CONNECT_ERROR;
	}
	return SUCCESS;
}

int Server::readMessage(char* buffer, int32_t size) {	
	//	Receive Header
	int32_t bytesRecvd = 0;
	int32_t result = 0;
	char length;
	result = recv(client, &length, 1, NULL);
	//	error checking
	if (result <= 0) {
		if (m_shutdown)
			return SHUTDOWN;
		else
			return DISCONNECT;
	}

	if (length > size)
		return PARAMETER_ERROR;

	//	Receive Message	
	while (bytesRecvd < (int32_t)length) {
		result = recv(client, buffer + bytesRecvd, (int32_t)length - bytesRecvd, 0);

		//	error checking
		if (result <= 0) {
			if (m_shutdown)
				return SHUTDOWN;
			else
				return DISCONNECT;
		}
		bytesRecvd += result;
	}
	return SUCCESS;
}

int Server::sendMessage(char* data, int32_t length) {
	if (length < 0 || length > 255)
		return PARAMETER_ERROR;

	//	Send Header
	char len = length;
	send(client, &len, 1, NULL);

	//	Send Message
	int32_t result = 0;
	result = send(client, data, length, NULL);

	//	error checking
	if (result <= 0) {
		if (m_shutdown)
			return SHUTDOWN;
		else
			return DISCONNECT;
	}
	return SUCCESS;
}

void Server::stop() {
	m_shutdown = true;
	shutdown(client, SD_BOTH);
	shutdown(server, SD_BOTH);
	closesocket(client);
	closesocket(server);
}