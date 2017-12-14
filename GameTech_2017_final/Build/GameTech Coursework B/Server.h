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
		, pathIdx(0)
		, avatarPosition(Vector2())
		, moveAvatar(false)
		, accumTime(0.0f)		
		, avatarPnode(new PhysicsNode())
	{
	}

	Client::~Client()
	{
		SAFE_DELETE(avatarPnode);
	}

	ENetPeer* peer;

	int startIdx;	//Start index into allNodes
	int endIdx;		//End index into allNodes
	int avatarIdx;	//Avatar index into allNodes
	int pathIdx;	//Avatar index into path indices

	Vector2 avatarPosition;
	bool moveAvatar;
	PhysicsNode* avatarPnode;

	std::vector<int> pathIndices;
	//Each client has its own timer to calculate when it needs to update its current path index
	//
	float accumTime;		
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
	const int FindIdx(const GraphNode* node);
	void GenerateMazeDataPacket(const std::string packetData, const char delim, const enet_uint16 clientID);
	void UpdateAvatars(const float dt);
	void UpdateAvatarVelocity(Client* client);

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

	float accumTime = 0.0f;
	float avatarSpeed = 3.0f;
};

