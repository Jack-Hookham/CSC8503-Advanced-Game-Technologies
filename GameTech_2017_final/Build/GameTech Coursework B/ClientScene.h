#pragma once

#include <ncltech\Scene.h>
#include <ncltech\NetworkBase.h>
#include <ncltech\PhysicsEngine.h>
#include <nclgl\NCLDebug.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\CommonUtils.h>
#include <nclgl\OBJMesh.h>

#include <iostream>
#include <sstream>
#include <iomanip>

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

	void GenerateNewMaze();

protected:
	void SendPacketToServer(const Packet& packet);
	void HandleKeyboardInputs();
	void UpdateStartPosition();
	void UpdateEndPosition();
	void RequestPath();
	void DrawPath(const std::vector<int>& finalPath, const float lineWidth);

	NetworkBase network;
	ENetPeer*	serverConnection;
	MazeGenerator* mazeGenerator;
	MazeRenderer* mazeRenderer;
	Mesh* wallMesh;

	GameObject* startNode;
	GameObject* endNode;

	int mazeSize;
	float mazeDensity;
	float mazeScalarf;
	Matrix4 mazeScalarMat4 = Matrix4();

	bool drawPath;
	std::vector<int> finalPath;
};
