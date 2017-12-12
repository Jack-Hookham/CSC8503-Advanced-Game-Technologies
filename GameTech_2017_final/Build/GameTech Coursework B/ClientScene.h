#pragma once

#include <ncltech\Scene.h>
#include <ncltech\NetworkBase.h>
#include <ncltech\PhysicsEngine.h>
#include <nclgl\NCLDebug.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\CommonUtils.h>"

#include <iostream>
#include <sstream>

#include "Packet.h"
#include "MazeGenerator.h"
#include "MazeRenderer.h"

const Vector3 pos_maze = Vector3(0.0f, 0.0f, 0.0f);

class ClientScene : public Scene
{
public:
	ClientScene(const std::string& friendly_name);

	virtual void OnInitializeScene() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdateScene(float dt) override;

	void ProcessNetworkEvent(const ENetEvent& evnt);

protected:
	void SendPacketToServer(const Packet& packet);

	GameObject* box;

	NetworkBase network;
	ENetPeer*	serverConnection;
	MazeGenerator* mazeGenerator;
	MazeRenderer* maze;
	Mesh* wallMesh;

	int mazeSize = 16;
	float mazeDensity = 0.5f;
};
