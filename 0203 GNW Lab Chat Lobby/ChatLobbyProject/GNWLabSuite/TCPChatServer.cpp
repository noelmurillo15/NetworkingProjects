#include "TCPChatServer.h"
#include "vld.h"

TCPChatServer::TCPChatServer(ChatLobby& chat_int) : chat_interface(chat_int) 
{

}

TCPChatServer::~TCPChatServer(void) 
{

}

bool TCPChatServer::init(uint16_t port) {

	numClients = 0;	
	ZeroMemory(clientList, sizeof(clientList));

	//	Create Server Socket
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
		return SOCKET_ERROR;

	address.sin_family = AF_INET;
	address.sin_addr.S_un.S_addr = INADDR_ANY;
	address.sin_port = htons(port);

	//	Bind 
	int result = bind(listenSocket, (sockaddr*)&address, sizeof(address));
	if (result == INVALID_SOCKET)
		return SOCKET_ERROR;

	//	Set up listening queue for connections
	result = listen(listenSocket, MAX_CLIENTS);
	if (result == INVALID_SOCKET)
		return SOCKET_ERROR;

	//	Add this socket to a stored fd_set 
	FD_ZERO(&masterset);
	FD_SET(listenSocket, &masterset);
	return true;
}

bool TCPChatServer::run(void) {

	//	Use select func to find ready sockets
	FD_ZERO(&readfds);
	readfds = masterset;
	select(NULL, &readfds, NULL, NULL, NULL);

	//	If listen socket is ready, accept the new client
	if (FD_ISSET(listenSocket, &readfds)) {
		newClient = accept(listenSocket, (sockaddr*)&address, NULL);
		if (newClient != INVALID_SOCKET) {
		FD_SET(newClient, &masterset);
			for (int x = 0; x < MAX_CLIENTS; x++) {
				if (clientList[x] == 0) {
					clients[x].m_id = x;
					clientList[x] = newClient;
					newClient = INVALID_SOCKET;
					return true;
				}
			}
		}
	}

	//	If we are full
	if (numClients == 4 && newClient != INVALID_SOCKET) {
		if (FD_ISSET(newClient, &readfds)) {
			short length;
			tcp_recv_whole(newClient, (char*)&length, 2, NULL);

			char type;
			tcp_recv_whole(newClient, &type, 1, NULL);

			if (type == cl_reg) {
				char buffer[3];
				short size = 1;
				memcpy(buffer, (char*)&size, sizeof(short));
				buffer[2] = sv_full;
				send(newClient, buffer, 3, NULL);
				FD_CLR(newClient, &masterset);
				closesocket(newClient);
				newClient = INVALID_SOCKET;
			}
		}
	}

	//	for each ready socket, parse incoming msgs
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (FD_ISSET(clientList[i], &readfds)) {
			short length;
			tcp_recv_whole(clientList[i], (char*)&length, 2, NULL);

			char type;
			tcp_recv_whole(clientList[i], &type, 1, NULL);

			//	Used to send a message back to server upon receiving
			short size = 0;
			int tmp = 0;

			//	Used to get users in the list
			char userID;
			char numList;
			char userName[17];

			//	Chat Message
			char chatMsg[256];
			std::string chatMsg_s;
			std::string displayString;

#pragma region Switch
			//	Handle messages based on type
			switch (type) {

			case cl_reg:
				if (numClients < MAX_CLIENTS) {					
					char buffer[4];
					size = 2; memcpy(buffer, (char*)&size, sizeof(short));
					buffer[2] = sv_cnt;
					buffer[3] = i;
					send(clientList[i], buffer, 4, NULL);

					tcp_recv_whole(clientList[i], userName, 17, NULL);
					clients[i].m_name = userName;

					char addBuffer[21];
					size = 19; memcpy(addBuffer, (char*)&size, sizeof(short));
					addBuffer[2] = sv_add;
					addBuffer[3] = i;
					strcpy(addBuffer + 4, userName);

					for (int x = 0; x < MAX_CLIENTS; x++)
						if (clientList[x] != 0 && x != i)
							send(clientList[x], addBuffer, 21, NULL);

					numClients++;
				}
				break;

			case cl_get:
				if (numClients == 1) {
					char buffer[22]; size = 20;
					memcpy(buffer, (char*)&size, sizeof(short));
					buffer[2] = sv_list;
					buffer[3] = numClients;
					buffer[4] = 0;
					strcpy(buffer + 5, clients[0].m_name.c_str());
					send(clientList[i], buffer, 22, NULL);
				}
				else if (numClients == 2) {
					char buffer[40]; size = 38;
					memcpy(buffer, (char*)&size, sizeof(short));
					buffer[2] = sv_list;
					buffer[3] = numClients;
					buffer[4] = 0;
					strcpy(buffer + 5, clients[0].m_name.c_str());
					buffer[22] = 1;
					strcpy(buffer + 23, clients[1].m_name.c_str());
					send(clientList[i], buffer, 40, NULL);
				}
				else if (numClients == 3) {
					char buffer[58]; size = 56;
					memcpy(buffer, (char*)&size, sizeof(short));
					buffer[2] = sv_list;
					buffer[3] = numClients;
					buffer[4] = 0;
					strcpy(buffer + 5, clients[0].m_name.c_str());
					buffer[22] = 1;
					strcpy(buffer + 23, clients[1].m_name.c_str());
					buffer[40] = 2;
					strcpy(buffer + 41, clients[2].m_name.c_str());
					send(clientList[i], buffer, 58, NULL);
				}
				else if (numClients == 4){
					char buffer[76]; size = 74;
					memcpy(buffer, (char*)&size, sizeof(short));
					buffer[2] = sv_list;
					buffer[3] = numClients;
					buffer[4] = 0;
					strcpy(buffer + 5, clients[0].m_name.c_str());
					buffer[22] = 1;
					strcpy(buffer + 23, clients[1].m_name.c_str());
					buffer[40] = 2;
					strcpy(buffer + 41, clients[2].m_name.c_str());
					buffer[58] = 3;
					strcpy(buffer + 59, clients[3].m_name.c_str());
					send(clientList[i], buffer, 76, NULL);
				}
				break;

			case sv_cl_msg:
			{
				tcp_recv_whole(clientList[i], &userID, 1, NULL);
				tcp_recv_whole(clientList[i], chatMsg, length - 2, NULL);
				size = length;
				length += 2;

				char* buffer = new char[length]();
				memcpy(buffer, (char*)&size, sizeof(short));
				buffer[2] = sv_cl_msg;
				buffer[3] = clients[i].m_id;
				strcpy(buffer + 4, chatMsg);

				for (int x = 0; x < MAX_CLIENTS; x++)
					if (clientList[x] != 0)
						send(clientList[x], buffer, length, NULL);

				delete[] buffer;
				break;
			}
			case sv_cl_close:
				tcp_recv_whole(clientList[i], &userID, 1, NULL);

				char buffer[4]; size = 2; 
				memcpy(buffer, (char*)&size, sizeof(short));
				buffer[2] = sv_remove; buffer[3] = userID;

				for (int x = 0; x < MAX_CLIENTS; x++)
					if (clientList[x] != 0)
						send(clientList[x], buffer, 4, NULL);

				clients[userID].m_id = -1;
				clients[userID].m_name = "";
				clientList[userID] = 0;
				numClients--;
				break;
			}
#pragma endregion
		}
	}

	//	close the passive socket
	return true;
}

bool TCPChatServer::stop(void) {
	char buffer[4];
	short size = 2;
	memcpy(buffer, (char*)&size, sizeof(short));
	buffer[2] = sv_cl_close;
	buffer[3] = 0;

	for (int x = 1; x < MAX_CLIENTS; x++)
		if (clientList[x] != 0)
			send(clientList[x], buffer, 4, NULL);

	shutdown(listenSocket, SD_BOTH);
	shutdown(masterClient, SD_BOTH);
	closesocket(listenSocket);
	closesocket(masterClient);
	return false;
}