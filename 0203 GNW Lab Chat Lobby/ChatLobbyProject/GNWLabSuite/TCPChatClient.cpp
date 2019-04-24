#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "TCPChatClient.h"

TCPChatClient::TCPChatClient(ChatLobby& chat_int) : chat_interface(chat_int)
{

}

TCPChatClient::~TCPChatClient(void)
{

}

bool TCPChatClient::init(std::string name, std::string ip_address, uint16_t port) {

	//	Establish a connected TCP socket endpt
	if (inet_addr(ip_address.c_str()) == INADDR_NONE)
		return false;

	sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.S_un.S_addr = inet_addr(ip_address.c_str());

	m_client = INVALID_SOCKET;
	m_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_client == INVALID_SOCKET)
		return false;

	int result = connect(m_client, (sockaddr*)&saddr, sizeof(sockaddr));
	if (result == INVALID_SOCKET)
		return false;

	//	Send off the register message to the server
	char regMsg[20];
	short length = 18;
	memcpy(regMsg, (char*)&length, sizeof(short));
	regMsg[2] = cl_reg;
	strcpy(regMsg + 3, name.c_str());
	if (send(m_client, regMsg, 20, NULL) == SOCKET_ERROR)
		return false;

	return true;
}

bool TCPChatClient::run(void) {

	//	Receive the generic message length header of 2 bytes
	short length;
	tcp_recv_whole(m_client, (char*)&length, 2, NULL);

	//	Receive what type of msg
	char type;
	tcp_recv_whole(m_client, &type, 1, NULL);

	//	Used to send a message back to server upon receiving
	short size = 0;

	//	Used to get users in the list
	char userID;
	char numList;
	char userName[17];

	//	Chat Message
	char chatMsg[256];
	std::string displayString;

#pragma region Switch
	//	Handle messages based on type
	switch (type) {

	case sv_cnt:
		chat_interface.DisplayString("CLIENT: CONNECTED TO SERVER");
		tcp_recv_whole(m_client, &userID, 1, NULL);
		m_id = userID;
		chat_interface.DisplayString("CLIENT: REGISTERED IN SERVER");

		//	Client should request list upon receiving sv_cnt from server
		char buffer[3];
		size = 1;

		memcpy(buffer, (char*)&size, sizeof(short));
		buffer[2] = cl_get;
		send(m_client, buffer, 3, NULL);
		break;

	case sv_full:
		chat_interface.DisplayString("CLIENT: SERVER IS FULL!");
		return stop();
		break;

	case sv_list:
		chat_interface.DisplayString("CLIENT: RECEIVED USER LIST");
		tcp_recv_whole(m_client, &numList, 1, NULL);
		for (int i = 0; i < numList; i++) {
			tcp_recv_whole(m_client, &userID, 1, NULL);
			tcp_recv_whole(m_client, userName, 17, NULL);
			chat_interface.AddNameToUserList(userName, userID);
		}
		break;

	case sv_add:
		tcp_recv_whole(m_client, &userID, 1, NULL);
		tcp_recv_whole(m_client, userName, 17, NULL);
		chat_interface.AddNameToUserList(userName, userID);		
		displayString = "CLIENT: ";
		displayString += userName;
		displayString += " JOINED";
		chat_interface.DisplayString(displayString);
		break;

	case sv_remove:
		tcp_recv_whole(m_client, &userID, 1, NULL);
		chat_interface.RemoveNameFromUserList(userID);
		chat_interface.DisplayString("CLIENT: SOMEONE HAS LEFT");
		break;

	case sv_cl_msg:
		tcp_recv_whole(m_client, &userID, 1, NULL);
		tcp_recv_whole(m_client, chatMsg, length - 2, NULL);
		chat_interface.AddChatMessage(chatMsg, userID);
		break;

	case sv_cl_close:
		tcp_recv_whole(m_client, &userID, 1, NULL);
		return stop();
		break;
	}
#pragma endregion

	return true;
}

bool TCPChatClient::send_message(std::string message) {

	char msg[256];

	short len = (short)(message.size() + 3);
	int totalSize = len + 2;

	ZeroMemory(&msg, sizeof(msg));
	memcpy(msg, (char*)&len, sizeof(short));

	msg[2] = sv_cl_msg; msg[3] = m_id;
	strcpy(msg + 4, message.c_str());

	if (send(m_client, msg, totalSize, NULL) == SOCKET_ERROR)
		return false;

	return true;
}

bool TCPChatClient::stop(void) {

	char shutoff[4];
	short len = 2;
	memcpy(shutoff, (char*)&len, sizeof(short));
	shutoff[2] = sv_cl_close;
	shutoff[3] = m_id;

	send(m_client, shutoff, 4, NULL);
	shutdown(m_client, SD_BOTH);
	closesocket(m_client);
	return false;
}