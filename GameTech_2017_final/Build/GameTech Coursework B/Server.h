#pragma once

#include <ncltech\CommonUtils.h>
#include <ncltech\NetworkBase.h>
#include <nclgl\common.h>
#include <nclgl\Vector3.h>

#include "MazeGenerator.h"
#include "SearchAStar.h"
#include "Packet.h"

#define UPDATE_TIMESTEP (1.0f / 30.0f) //send 30 position updates per second
#define MAX_CLIENTS 32

struct Client
{
	Client::Client(ENetPeer* peer)
		: peer(peer)
		, startIdx(0)
		, endIdx(0)
		, avatarIdx(0)
		, avatarPosition(Vector2())
	{
	}

	ENetPeer* peer;

	int startIdx;
	int endIdx;
	int avatarIdx;

	Vector2 avatarPosition;
};

class Server
{
public:
	Server();
	~Server();

	void RunServer();

	inline NetworkBase GetNetworkBase() { return networkBase; }
	inline bool Initialize(uint16_t port, size_t maxPeers) { return networkBase.Initialize(port, maxPeers); }
	inline void Release() { networkBase.Release(); }

private:
	void SendPacketToClients(const Packet& packet);
	void SendPacketToClient(ENetPeer* peer, const Packet& packet);
	std::string FindNode(const GraphNode* node);
	void GenerateMazeDataPacket(const std::string packetData, const char delim, const enet_uint16 clientID);

	Client* clients[MAX_CLIENTS];

	GameTimer timer;
	NetworkBase networkBase;
	MazeGenerator* mazeGenerator;
	SearchAStar* searchAStar;

	int mazeSize;
	float mazeDensity;
	//Store a packet containing the current maze structure so that it can be passed to any new clients
	Packet* mazeDataPacket;
	//Store another packet containing the current maze size and density so that when one client updates the params
	//they are updated for all clients
	Packet* mazeParamsPacket;
};

