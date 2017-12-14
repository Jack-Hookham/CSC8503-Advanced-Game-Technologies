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

#define MAX_MAZE_SIZE 50

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
	void UpdateAvatarServerPosition();

protected:
	void SendPacketToServer(const Packet& packet);
	void HandleKeyboardInputs();
	void UpdateStartPosition();
	void UpdateEndPosition();
	void RequestPath();
	void DrawPath(const std::vector<int>& finalPath, const float lineWidth);
	void SendMazeParamsPacket();

	NetworkBase network;
	ENetPeer*	serverConnection;
	MazeGenerator* mazeGenerator;
	MazeRenderer* mazeRenderer;
	Mesh* wallMesh;

	GameObject* startNode;
	GameObject* endNode;
	//GameObject* clientGameObj;
	GameObject* clientGameObjs[MAX_CLIENTS];
	RenderNode* clientRnodes[MAX_CLIENTS];		//Store render nodes for all clients
	int clientID;								//Store this client's ID
	int avatarIdx;
	int clientsConnected;						//Includes self

	int mazeSize;
	float mazeDensity;
	float mazeScalarf;
	Matrix4 mazeScalarMat4 = Matrix4();

	//Having two draw path bools means that when a new maze is generated the actual
	//draw path bool is turned off but wantToDrawPath can be left as it was
	//Then when a path packet is received drawPath is set to wantToDrawPath
	//The actual drawing of the path is determined by drawPath
	//I did this because when generating a new maze the time between receiving the
	//maze data and the new path data meant that the maze data and path data were 
	//out of sync temporarily so a squiggle was drawn

	//Whether the path is actually drawn
	bool drawPath;
	//Whether the client wants to draw the path
	bool wantToDrawPath;

	std::vector<int> finalPath;
	int pathIndex;

	//Same as drawing
	//if a node is moved then stop moving while we wait for the new path
	bool isMove;
	bool wantToMove;

	//Update movement bool and send it to the server
	void UpdateIsMove(const bool move)
	{
		isMove = move;
		//Send the new move bool to the server
		Packet isMovePacket(PacketType::PACKET_IS_MOVE);
		isMovePacket.SetData(to_string(isMove));
		SendPacketToServer(isMovePacket);
	}
};
