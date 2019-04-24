// Server.cpp : Contains all functions of the server.
#include "Server.h"

// Initializes the server. (NOTE: Does not wait for player connections!)
int Server::init(uint16_t port) {
	initState();

	//	1) Set up a socket for listening.
	svSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (svSocket == INVALID_SOCKET)
		return SETUP_ERROR;

	memset((char*)&playerAddress[0], 0, sizeof(playerAddress[0]));
	memset((char*)&playerAddress[1], 0, sizeof(playerAddress[1]));
	playerAddress[0].sin_family = AF_INET;
	playerAddress[0].sin_port = htons(port);
	playerAddress[0].sin_addr.S_un.S_addr = INADDR_ANY;

		//	Bind Socket to port
	int result = bind(svSocket, (SOCKADDR*)&playerAddress[0], sizeof(playerAddress[0]));
	if (result == SOCKET_ERROR) 
		return ADDRESS_ERROR;

	unsigned long val = 1;
	if (ioctlsocket(svSocket, FIONBIO, &val) == SOCKET_ERROR)
		return DISCONNECT;

	//	2) Mark the server as active.
	sequence0 = 0;
	sequence1 = 0;
	active = true;

	return SUCCESS;
}

// Updates the server; called each game "tick".
int Server::update() {
	//	1) Get player input and process it.
		//	Allocate buffer into which msgs can be read
	NetworkMessage msg(IO::_INPUT);
		//	Allocate socket address to store the src of msgs
	sockaddr_in saddr;
		//	While there are msgs to store the source of msgs
	while (recvfromNetMessage(svSocket, msg, &saddr) >= 0) {
			//	Read the next incoming msg into the buffer
			//	If there is an error caused by a network problem, return Disconnect
			//	If there is an error caused by a server shutdown, return Shutdown
			//	If successful, parse the new msg
		parseMessage(saddr, msg);
	}
		
	//	2) If any player's timer exceeds 50, "disconnect" the player.
	if (playerTimer[0] > 50)
		disconnectClient(0);
	if (playerTimer[1] > 50)
		disconnectClient(1);
	//	3) Update the state and send the current snapshot to each player.
		//	Send the latest snapshot to each client
	updateState();
	sendState();

	return SUCCESS;
}

// Stops the server.
void Server::stop() {
	//       1) Sends a "close" message to each client.
	sendClose();
	//       2) Shuts down the server gracefully (update method should exit with SHUTDOWN code.)
	shutdown(svSocket, SD_BOTH);
	close(svSocket);
}

// Parses a message and responds if necessary. (private, suggested)
int Server::parseMessage(sockaddr_in& source, NetworkMessage& message) {
	char type = message.readByte();
	//	If the msg is a 'connect' see if that player slot is available
	switch (type) {
	case CL_CONNECT:
		//	If so, send an 'okay' msg to the src
		if (noOfPlayers == 0) {
			connectClient(0, source);
			sendOkay(source);
		}
		else if (noOfPlayers == 1) {
			connectClient(1, source);
			sendOkay(source);
		}
		//	If not, send a 'full' msg to the src
		else
			sendFull(source);

		return SUCCESS;
	}

	//	Otherwise, determine which player the msg is from
	int player = -1;
	if (source.sin_port == playerAddress[0].sin_port)
		player = 0;
	else if (source.sin_port == playerAddress[1].sin_port)
		player = 1;
	
	switch (type) {
	//	If msg is Input, apply the key changes to the proper player's input state
	case CL_KEYS:		
		if (player == 0) {
			state.player0.keyUp = message.readByte();
			state.player0.keyDown = message.readByte();
		}
		else if (player == 1) {
			state.player1.keyUp = message.readByte();
			state.player1.keyDown = message.readByte();
		}
		break;

	//	If msg is Alive, reset the proper player's player timer
	case CL_ALIVE:
		playerTimer[player] = 0;
		break;

	//	If msg is Close, disconnect the proper player
	case SV_CL_CLOSE:
		disconnectClient(player);
		break;
	}
	return SUCCESS;
}

// Sends the "SV_OKAY" message to destination. (private, suggested)
int Server::sendOkay(sockaddr_in& destination) {
	// TODO: Send "SV_OKAY" to the destination.
	NetworkMessage msg(IO::_OUTPUT);
	if (destination.sin_port == playerAddress[0].sin_port) {
		sequence0++;
		msg.writeShort(sequence0);
	}
	else if (destination.sin_port == playerAddress[1].sin_port) {
		sequence1++;
		msg.writeShort(sequence1);
	}
	msg.writeByte(SV_OKAY);
	sendtoNetMessage(svSocket, msg, &destination);
	return SUCCESS;
}

// Sends the "SV_FULL" message to destination. (private, suggested)
int Server::sendFull(sockaddr_in& destination) {
	// TODO: Send "SV_FULL" to the destination.
	NetworkMessage msg(IO::_OUTPUT);
	if (destination.sin_port == playerAddress[0].sin_port) {
		sequence0++;
		msg.writeShort(sequence0);
	}
	else if (destination.sin_port == playerAddress[1].sin_port) {
		sequence1++;
		msg.writeShort(sequence1);
	}
	msg.writeByte(SV_FULL);
	sendtoNetMessage(svSocket, msg, &destination);
	return SUCCESS;
}

// Sends the current snapshot to all players. (private, suggested)
int Server::sendState() {
	NetworkMessage msg1(IO::_OUTPUT);
	NetworkMessage msg0(IO::_OUTPUT);
	sequence0++;
	sequence1++;

	msg0.writeShort(sequence0);
	msg0.writeByte(SV_SNAPSHOT);
	msg0.writeByte(state.gamePhase);
	msg0.writeShort(state.ballX);
	msg0.writeShort(state.ballY);
	msg0.writeShort(state.player0.y);
	msg0.writeShort(state.player0.score);
	msg0.writeShort(state.player1.y);
	msg0.writeShort(state.player1.score);

	msg1.writeShort(sequence1);
	msg1.writeByte(SV_SNAPSHOT);
	msg1.writeByte(state.gamePhase);
	msg1.writeShort(state.ballX);
	msg1.writeShort(state.ballY);
	msg1.writeShort(state.player0.y);
	msg1.writeShort(state.player0.score);
	msg1.writeShort(state.player1.y);
	msg1.writeShort(state.player1.score);

	sendtoNetMessage(svSocket, msg0, &playerAddress[0]);
	sendtoNetMessage(svSocket, msg1, &playerAddress[1]);
	return SUCCESS;
}

// Sends the "SV_CL_CLOSE" message to all clients. (private, suggested)
void Server::sendClose() {
	NetworkMessage msg0(IO::_OUTPUT);
	NetworkMessage msg1(IO::_OUTPUT);

	sequence0++;
	msg0.writeShort(sequence0);
	msg0.writeByte(SV_CL_CLOSE);

	sequence1++;
	msg1.writeShort(sequence1);
	msg1.writeByte(SV_CL_CLOSE);

	sendtoNetMessage(svSocket, msg0, &playerAddress[0]);
	sendtoNetMessage(svSocket, msg1, &playerAddress[1]);
}

// Server message-sending helper method. (private, suggested)
int Server::sendMessage(sockaddr_in& destination, NetworkMessage& message) {
	// TODO: Send the message in the buffer to the destination.s
	return SUCCESS;
}

// Marks a client as connected and adjusts the game state.
void Server::connectClient(int player, sockaddr_in& source)
{
	playerAddress[player] = source;
	playerTimer[player] = 0;

	noOfPlayers++;
	if (noOfPlayers == 1)
		state.gamePhase = WAITING;
	else
		state.gamePhase = RUNNING;
}

// Marks a client as disconnected and adjusts the game state.
void Server::disconnectClient(int player)
{
	playerAddress[player].sin_addr.s_addr = INADDR_NONE;
	playerAddress[player].sin_port = 0;

	noOfPlayers--;
	if (noOfPlayers == 1)
		state.gamePhase = WAITING;
	else
		state.gamePhase = DISCONNECTED;
}

// Updates the state of the game.
void Server::updateState()
{
	// Tick counter.
	static int timer = 0;

	// Update the tick counter.
	timer++;

	// Next, update the game state.
	if (state.gamePhase == RUNNING)
	{
		// Update the player tick counters (for ALIVE messages.)
		playerTimer[0]++;
		playerTimer[1]++;

		// Update the positions of the player paddles
		if (state.player0.keyUp)
			state.player0.y--;
		if (state.player0.keyDown)
			state.player0.y++;

		if (state.player1.keyUp)
			state.player1.y--;
		if (state.player1.keyDown)
			state.player1.y++;
		
		// Make sure the paddle new positions are within the bounds.
		if (state.player0.y < 0)
			state.player0.y = 0;
		else if (state.player0.y > FIELDY - PADDLEY)
			state.player0.y = FIELDY - PADDLEY;

		if (state.player1.y < 0)
			state.player1.y = 0;
		else if (state.player1.y > FIELDY - PADDLEY)
			state.player1.y = FIELDY - PADDLEY;

		//just in case it get stuck...
		if (ballVecX)
			storedBallVecX = ballVecX;
		else
			ballVecX = storedBallVecX;

		if (ballVecY)
			storedBallVecY = ballVecY;
		else
			ballVecY = storedBallVecY;

		state.ballX += ballVecX;
		state.ballY += ballVecY;

		// Check for paddle collisions & scoring
		if (state.ballX < PADDLEX)
		{
			// If the ball has struck the paddle...
			if (state.ballY + BALLY > state.player0.y && state.ballY < state.player0.y + PADDLEY)
			{
				state.ballX = PADDLEX;
				ballVecX *= -1;
			}
			// Otherwise, the second player has scored.
			else
			{
				newBall();
				state.player1.score++;
				ballVecX *= -1;
			}
		}
		else if (state.ballX >= FIELDX - PADDLEX - BALLX)
		{
			// If the ball has struck the paddle...
			if (state.ballY + BALLY > state.player1.y && state.ballY < state.player1.y + PADDLEY)
			{
				state.ballX = FIELDX - PADDLEX - BALLX - 1;
				ballVecX *= -1;
			}
			// Otherwise, the first player has scored.
			else
			{
				newBall();
				state.player0.score++;
				ballVecX *= -1;
			}
		}

		// Check for Y position "bounce"
		if (state.ballY < 0)
		{
			state.ballY = 0;
			ballVecY *= -1;
		}
		else if (state.ballY >= FIELDY - BALLY)
		{
			state.ballY = FIELDY - BALLY - 1;
			ballVecY *= -1;
		}
	}

	// If the game is over...
	if ((state.player0.score > 10 || state.player1.score > 10) && state.gamePhase == RUNNING)
	{
		state.gamePhase = GAMEOVER;
		timer = 0;
	}
	if (state.gamePhase == GAMEOVER)
	{
		if (timer > 30)
		{
			initState();
			state.gamePhase = RUNNING;
		}
	}
}

// Initializes the state of the game.
void Server::initState()
{
	playerAddress[0].sin_addr.s_addr = INADDR_NONE;
	playerAddress[1].sin_addr.s_addr = INADDR_NONE;
	playerTimer[0] = playerTimer[1] = 0;
	noOfPlayers = 0;

	state.gamePhase = DISCONNECTED;

	state.player0.y = 0;
	state.player1.y = FIELDY - PADDLEY - 1;
	state.player0.score = state.player1.score = 0;
	state.player0.keyUp = state.player0.keyDown = false;
	state.player1.keyUp = state.player1.keyDown = false;

	newBall(); // Get new random ball position

	// Get a new random ball vector that is reasonable
	ballVecX = (rand() % 10) - 5;
	if ((ballVecX >= 0) && (ballVecX < 2))
		ballVecX = 2;
	if ((ballVecX < 0) && (ballVecX > -2))
		ballVecX = -2;

	ballVecY = (rand() % 10) - 5;
	if ((ballVecY >= 0) && (ballVecY < 2))
		ballVecY = 2;
	if ((ballVecY < 0) && (ballVecY > -2))
		ballVecY = -2;
}

// Places the ball randomly within the middle half of the field.
void Server::newBall()
{
	// (randomly in 1/2 playable area) + (1/4 playable area) + (left buffer) + (half ball)
	state.ballX = (rand() % ((FIELDX - 2 * PADDLEX - BALLX) / 2)) +
						((FIELDX - 2 * PADDLEX - BALLX) / 4) + PADDLEX + (BALLX / 2);

	// (randomly in 1/2 playable area) + (1/4 playable area) + (half ball)
	state.ballY = (rand() % ((FIELDY - BALLY) / 2)) + ((FIELDY - BALLY) / 4) + (BALLY / 2);
}