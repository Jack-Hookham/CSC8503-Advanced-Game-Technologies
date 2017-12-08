#pragma once

#include <ncltech\NetworkBase.h>

class Server
{
public:
	Server();
	~Server();

	void ServerLoop();

	inline NetworkBase serverNetworkBase() { return networkBase; }

private:

	NetworkBase networkBase;
};

