#include "../platform.h"
#include "../definitions.h"

class Client {

	int player;
	short sequence;
	SOCKET clSocket;
	volatile bool active;//controls the activity of the clients run which is on its own thread.
	static int snapCount;

	GameState state;
	CriticalSection cs;//just incase you need to protect data, you might not need to.

public:
	inline Client() { state.gamePhase = WAITING; }

	int init(char* address, uint16_t port, uint8_t player);
	int run();
	void stop();

	int sendInput(int8_t keyUp, int8_t keyDown, int8_t keyQuit);
	void getState(GameState* target);
	
private:
	int sendAlive();
	void sendClose();
};