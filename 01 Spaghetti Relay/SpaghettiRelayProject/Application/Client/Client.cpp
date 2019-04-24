#include "Client.h"


Client::Client() {

}

Client::~Client() {

}

int Client::init(uint16_t port, char* address) {
	//	If addr is not dotted-quadrant format
	if (inet_addr(address) == INADDR_NONE)
		return ADDRESS_ERROR;
	
	m_shutdown = false;
	sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.S_un.S_addr = inet_addr(address);

	//	Create Socket & check for socket error
	sock = INVALID_SOCKET;
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
		return SETUP_ERROR;

	//	Connect the socket to the sock_addr
	int result = connect(sock, (sockaddr*)&saddr, sizeof(sockaddr));
	if (result == INVALID_SOCKET) {
		if (m_shutdown)
			return SHUTDOWN;
		else
			return CONNECT_ERROR;
	}
	//	Successful connection
	return SUCCESS;
}

int Client::readMessage(char* buffer, int32_t size) {
	//	Receive Header
	int32_t bytesRecvd = 0;
	int32_t result = 0;
	char length;
	result = recv(sock, &length, 1, NULL);

	//	error checking
	if (result <= 0) {
		if (m_shutdown)
			return SHUTDOWN;
		else
			return DISCONNECT;
	}

	if (length < 0 || length > size)
		return PARAMETER_ERROR;

	//	Receive Message
	while (bytesRecvd < (int32_t)length) {
		result = recv(sock, buffer + bytesRecvd, (int32_t)length - bytesRecvd, 0);

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

int Client::sendMessage(char* data, int32_t length) {
	if (length < 0 || length > 255)
		return PARAMETER_ERROR;

	//	Send Header
	char len = length;
	send(sock, &len, 1, NULL);

	//	Send Message
	int32_t result = 0;
	result = send(sock, data, length, NULL);

	//	error checking
	if (result <= 0) {
		if (m_shutdown)
			return SHUTDOWN;
		else
			return DISCONNECT;
	}
	return SUCCESS;
}

void Client::stop() {
	m_shutdown = true;
	shutdown(sock, SD_BOTH);
	closesocket(sock);
}