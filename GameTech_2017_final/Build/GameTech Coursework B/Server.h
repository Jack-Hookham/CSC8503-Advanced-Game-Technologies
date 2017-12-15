#pragma once

#include <ncltech\CommonUtils.h>
#include <ncltech\NetworkBase.h>
#include <nclgl\common.h>
#include <nclgl\Vector3.h>

#include "MazeGenerator.h"
#include "SearchAStar.h"
#include "Packet.h"

#define UPDATE_TIMESTEP (1.0f / 30.0f) //send 30 position updates per second
#define AVATAR_SPEED 2.0f

struct Client
{
	Client::Client(ENetPeer* peer)
		: peer(peer)
		, startIdx(0)
		, endIdx(0)
		, avatarIdx(0)
		, pathIdx(0)
		, avatarPosition(Vector2())
		, isMove(false)
		, pathTime(0.0f)
		, sendUpdateTime(0.0f)
		, avatarPnode(new PhysicsNode())
		, useStringPulling(false)
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
	bool isMove;
	PhysicsNode* avatarPnode;

	std::vector<int> finalPathAS;
	std::vector<int> finalPathSP;
	//Each client has its own timer to calculate when it needs to update its current path index
	float pathTime;	
	//And a timer to determine whether it needs to send the updated position to the client
	//Currently every 1 / 30 second
	float sendUpdateTime;

	bool useStringPulling;
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
};

