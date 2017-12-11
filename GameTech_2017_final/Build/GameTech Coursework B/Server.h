#pragma once

#include <ncltech\NetworkBase.h>
#include <nclgl\common.h>
#include <nclgl\Vector3.h>

#define UPDATE_TIMESTEP (1.0f / 30.0f) //send 30 position updates per second

class Server
{
public:
	Server();
	~Server();

	void Update(float dt);

	inline NetworkBase GetNetworkBase() { return networkBase; }
	bool Initialize(uint16_t port, size_t maxPeers)
	{
		return networkBase.Initialize(port, maxPeers);
	}

private:

	NetworkBase networkBase;
	float accum_time = 0.0f;
	float rotation = 0.0f;
};

