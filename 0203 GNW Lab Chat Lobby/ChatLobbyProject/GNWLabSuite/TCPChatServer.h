#pragma once
#include "ChatLobby.h"//Interface With Game Functionality
#include "NetDefines.h"
#include <stdint.h>//specifided data length types included (uint16_t)
#include <map>

#define MAX_CLIENTS 4


class TCPChatServer {

	struct USER {
		int m_id;
		std::string m_name;
		USER() = default;
		USER(std::string name, int id) { m_name = name; m_id = id; }
	};

	int numClients;
	USER clients[MAX_CLIENTS];
	int clientList[MAX_CLIENTS];

	int listenSocket;
	int masterClient;
	int newClient;

	fd_set readfds;
	fd_set masterset;
	sockaddr_in address;

	ChatLobby& chat_interface;

public:

	TCPChatServer(ChatLobby& chat_int);
	~TCPChatServer(void);
	//Establishes a listening socket, Only Called once in Separate Server Network Thread, Returns true aslong as listening can and has started on the proper address and port, otherwise return false
	bool init(uint16_t port);
	//Recieves data from clients, parses it, responds accordingly, Only called in Separate Server Network Thread, Will be continuously called until return = false;
	bool run(void);
	//Notfies clients of close & closes down sockets, Can be called on multiple threads
	bool stop(void);
};