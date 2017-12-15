#include "ClientScene.h"

const Vector3 status_color3 = Vector3(1.0f, 0.6f, 0.6f);
const Vector4 status_color = Vector4(status_color3.x, status_color3.y, status_color3.z, 1.0f);

ClientScene::ClientScene(const std::string& friendly_name)
	: Scene(friendly_name)
	, serverConnection(NULL)
	, mazeGenerator(NULL)
	, mazeRenderer(NULL)
	, wallMesh(NULL)
	, startNode(NULL)
	, endNode(NULL)
	, clientGameObjs{ NULL }
	, clientID(-1)
	, clientsConnected(0)
	, clientRnodes{ NULL }
	, mazeSize(16)
	, mazeDensity(1.0f)
	, mazeScalarf(1.0f)
	, mazeScalarMat4(Matrix4())
	, drawPath(false)
	, wantToDrawPath(false)
	, isMove(false)
	, wantToMove(false)
	, avatarIdx(0)
	, useStringPulling(false)
	, avatarSize(Vector3())
	, cellsize(Vector3())
{
}

void ClientScene::OnInitializeScene()
{
	wallMesh = new OBJMesh(MESHDIR"cube.obj");

	GLuint whitetex;
	glGenTextures(1, &whitetex);
	glBindTexture(GL_TEXTURE_2D, whitetex);
	unsigned int pixel = 0xFFFFFFFF;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixel);
	glBindTexture(GL_TEXTURE_2D, 0);

	wallMesh->SetTexture(whitetex);

	GraphicsPipeline::Instance()->GetCamera()->SetPosition(Vector3(0.0f, 8.0f, 1.5f));
	GraphicsPipeline::Instance()->GetCamera()->SetPitch(-70.0f);
	GraphicsPipeline::Instance()->GetCamera()->SetYaw(0.0f);

	mazeGenerator = new MazeGenerator();

	//Initialize Client Network
	if (network.Initialize(0))
	{
		NCLDebug::Log("Network: Initialized!");

		//Attempt to connect to the server on localhost:1234
		serverConnection = network.ConnectPeer(127, 0, 0, 1, 1234);
		NCLDebug::Log("Network: Attempting to connect to server.");
	}

	//Create Ground
	this->AddGameObject(CommonUtils::BuildCuboidObject(
		"Ground",
		Vector3(0.0f, -1.0f, 0.0f),
		Vector3(20.0f, 1.0f, 20.0f),
		true,
		0.0f,
		true,
		false,
		Vector4(0.2f, 0.5f, 1.0f, 1.0f),
		false,
		CommonMeshes::MeshType::DEFAULT_CUBE));

	//Seed the start and end positions with the current time so that each client
	//uses a different seed
	srand(time(0));
}

void ClientScene::OnCleanupScene()
{
	//delete all render nodes if they aren't set to be deleted by DeleteAllGameObjects
	//if the avatar obj's RenderNode is set to NULL then the avatarRender RenderNode 
	//won't be deleted by DeleteAllGameObjects so delete it here
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientGameObjs[i])
		{
			if (clientGameObjs[i]->HasRender())
			{
				if (clientID >= 0)
				{
					clientRnodes[i] = NULL;
				}
			}
			else
			{
				if (clientID >= 0)
				{
					SAFE_DELETE(clientRnodes[i]);
				}
			}
		}
	}
	Scene::OnCleanupScene();

				//Send one final packet telling the server we are disconnecting
				// - We are not waiting to resend this, so if it fails to arrive
				//   the server will have to wait until we time out naturally
	enet_peer_disconnect_now(serverConnection, 0);

	//Release network and all associated data/peer connections
	network.Release();
	serverConnection = NULL;
	SAFE_DELETE(wallMesh);
	SAFE_DELETE(mazeGenerator);
	mazeRenderer = NULL;
}

void ClientScene::OnUpdateScene(float dt)
{
	Scene::OnUpdateScene(dt);

	//Update Network
	auto callback = std::bind(
		&ClientScene::ProcessNetworkEvent,	// Function to call
		this,								// Associated class instance
		std::placeholders::_1);				// Where to place the first parameter
	network.ServiceNetwork(dt, callback);

	//Add Debug Information to screen
	uint8_t ip1 = serverConnection->address.host & 0xFF;
	uint8_t ip2 = (serverConnection->address.host >> 8) & 0xFF;
	uint8_t ip3 = (serverConnection->address.host >> 16) & 0xFF;
	uint8_t ip4 = (serverConnection->address.host >> 24) & 0xFF;

	NCLDebug::AddStatusEntry(status_color, "Client ID: %d", clientID);
	NCLDebug::AddStatusEntry(status_color, "Clients Connected: %d", clientsConnected);
	NCLDebug::AddStatusEntry(status_color, "Network Traffic");
	NCLDebug::AddStatusEntry(status_color, "    Incoming: %5.2fKbps", network.m_IncomingKb);
	NCLDebug::AddStatusEntry(status_color, "    Outgoing: %5.2fKbps", network.m_OutgoingKb);
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "--- Controls ---");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Generate New Maze [G]", mazeSize);
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Draw Path : %s [H] to toggle)", wantToDrawPath ? "On" : "Off");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Spawn Avatar [J]");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Move Avatar : %s [K] to toggle)", isMove ? "On" : "Off");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   String Pulling : %s [L] to toggle)", useStringPulling ? "On" : "Off");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Grid Size : %2d ([1]/[2] to change)", mazeSize);
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Density : %2.0f percent ([3]/[4] to change)", mazeDensity * 100.f);
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Move Start Node [Arrow Keys]");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Move End Node [CTRL + Arrow Keys]");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "");

	NCLDebug::AddStatusEntry(Vector4(status_color), "--- Debug ---");
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << GraphicsPipeline::Instance()->GetCamera()->GetPosition();
	std::string s = "Camera Position: " + oss.str();
	NCLDebug::AddStatusEntry(status_color, s);
	NCLDebug::AddStatusEntry(Vector4(status_color), "   Start Index: %d", mazeGenerator->GetStartIdx());
	NCLDebug::AddStatusEntry(Vector4(status_color), "   End Index: %d", mazeGenerator->GetEndIdx());
	NCLDebug::AddStatusEntry(Vector4(status_color), "   Avatar Index: %d", avatarIdx);
	NCLDebug::AddStatusEntry(Vector4(status_color), "   A* nodes: %d", aStarNodes);
	NCLDebug::AddStatusEntry(Vector4(status_color), "   String pulling nodes: %s", useStringPulling ? std::to_string(stringPullingNodes).c_str() : "N/A");

	HandleKeyboardInputs();

	if (mazeRenderer && mazeGenerator)
	{
		if (drawPath)
		{
			if (useStringPulling)
			{
				DrawPath(finalPathSP, 2.5f / float(mazeSize));
			}
			else
			{
				DrawPath(finalPathAS, 2.5f / float(mazeSize));
			}
		}
	}

	//Update the number of connected clients
	if (mazeRenderer && mazeGenerator)
	{
		clientsConnected = 0;
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (clientGameObjs[i]->Render())
			{
				clientsConnected++;
			}
		}
	}
}

void ClientScene::ProcessNetworkEvent(const ENetEvent& evnt)
{
	switch (evnt.type)
	{
	//New connection request or an existing peer accepted our connection request
	case ENET_EVENT_TYPE_CONNECT:
	{
		if (evnt.peer == serverConnection)
		{
			NCLDebug::Log(status_color3, "Network: Successfully connected to server!");

			//Send a 'hello' packet
			Packet msgPacket(PACKET_MESSAGE);
			msgPacket.SetData(std::string("Hello"));
			SendPacketToServer(msgPacket);
		}
	}
	break;

	//Server has sent us a new packet
	case ENET_EVENT_TYPE_RECEIVE:
	{
		std::string packetString((char*)evnt.packet->data);

		//Split the packet into string tokens
		char delim = ' ';

		//Separate first token (should be packet type) and rest of packet (packet data)
		std::string firstToken = packetString.substr(0, packetString.find(delim));
		std::string packetData = packetString.substr(packetString.find_first_of(delim) + 1);
		
		uint packetType;
		//The first token should be the packet type
		if (CommonUtils::isInteger(firstToken))
		{
			packetType = std::stoi(firstToken);
		}
		else
		{
			packetType = PacketType::PACKET_BAD;
		}

		switch(packetType)
		{
			case PacketType::PACKET_BAD:
			{
				std::cout << "\t Failed to read packet from Server. Unknown packet type " << packetType << ".\n";
				break;
			}
			case PacketType::PACKET_MESSAGE:
			{
				//Print the message
				std::cout << "\t Server broadcast: " << packetData << "\n";
				break;
			}
			case PacketType::PACKET_MAZE_PARAMS:
			{
				//Update the clients parameters
				//Split the data into its (hopefully) 2 ints
				std::vector<std::string> packetTokens;
				std::stringstream ss(packetData);
				string token;
				while (getline(ss, token, delim))
				{
					packetTokens.push_back(token);
				}

				if (CommonUtils::isInteger(packetTokens[0]) && CommonUtils::isFloat(packetTokens[1]))
				{
					std::cout << "\t Updated maze parameters from server.\n";
					mazeSize = std::stoi(packetTokens[0]);
					mazeDensity = std::atof(packetTokens[1].c_str());
				}
				else
				{
					std::cout << "\t Failed to process maze parameters from server.\n";
					return;
				}

				break;
			}
			case PacketType::PACKET_MAZE_DATA:
			{
				const uint dataLength = mazeSize * (mazeSize - 1) * 2;
				//Ensure that data string isn't longer than the required length
				packetData.resize(dataLength);

				//Process maze data packet, generate and render maze
				bool* isWall = new bool[dataLength];
				int idx = 0;
				for (auto data : packetData)
				{
					isWall[idx] = (data == '1');
					idx++;
				}

				//Generate the maze from the packet data received from the client
				//Generate a maze with density 0
				mazeGenerator->Generate(mazeSize, 0.0f);

				//When a new maze is generated the server needs to update the avatars position to the client's 
				//time seeded start position
				//The start and end positions are updated on the server by RequestPath(), but this doesn't update
				//the avatar position
				avatarIdx = mazeGenerator->GetStartIdx();
				UpdateAvatarServerPosition();

				//Populate the wall data with the data from the server
				GraphEdge* allEdges = mazeGenerator->GetAllEdgesArr();
				uint base_offset = mazeSize * (mazeSize - 1);
				for (uint y = 0; y < mazeSize; ++y)
				{
					for (uint x = 0; x < mazeSize - 1; ++x)
					{
						GraphEdge* edgeX = &allEdges[(y * (mazeSize - 1) + x)];
						edgeX->_iswall = isWall[(y * (mazeSize - 1) + x)];
					}
				}
				for (uint y = 0; y < mazeSize - 1; ++y)
				{
					for (uint x = 0; x < mazeSize; ++x)
					{
						GraphEdge* edgeY = &allEdges[base_offset + (x * (mazeSize - 1) + y)];
						edgeY->_iswall = isWall[base_offset + (x * (mazeSize - 1) + y)];
					}
				}

				GenerateNewMaze();

				delete[] isWall;
				break;
			}
			case PacketType::PACKET_PATH_AS:
			{
				std::cout << "\t Updating A* path with nodes: " << packetData << ".\n";

				//Update the path to be rendered
				//Clear the current path
				finalPathAS.clear();
				//Convert packet string data to indecies of the all nodes array
				std::istringstream iss(packetData);
				int pathVal;
				while (iss >> pathVal)
				{
					finalPathAS.push_back(pathVal);
				}

				break;
			}
			case PacketType::PACKET_PATH_SP:
			{
				std::cout << "\t Updating string pulling path with nodes: " << packetData << ".\n";

				//Update the path to be rendered
				//Clear the current path
				finalPathSP.clear();
				//Convert packet string data to indecies of the all nodes array
				std::istringstream iss(packetData);
				int pathVal;
				while (iss >> pathVal)
				{
					finalPathSP.push_back(pathVal);
				}

				drawPath = wantToDrawPath;
				UpdateIsMove(wantToMove);

				break;
			}
			case PacketType::PACKET_UPDATE_AVATAR_POS:
			{
				//Update the client's avatar position
				//Split the data into its (hopefully) 2 floats
				std::vector<std::string> packetTokens;
				std::stringstream ss(packetData);
				string token;
				while (getline(ss, token, delim))
				{
					packetTokens.push_back(token);
				}

				if (CommonUtils::isInteger(packetTokens[0]) && CommonUtils::isFloat(packetTokens[1]) && CommonUtils::isFloat(packetTokens[2]))
				{
					int updateID = std::stoi(packetTokens[0]);			//ID of the client to be updated
					float posX = std::atof(packetTokens[1].c_str());
					float posY = std::atof(packetTokens[2].c_str());

					Vector3 cellpos = Vector3(
						posX * 3.0f,
						0.0f,
						posY * 3.0f
					) * mazeScalarf;
					Vector3 cellsize = Vector3(
						mazeScalarf * 2.0f,
						1.0f,
						mazeScalarf * 2.0f
					);
					Vector3 avatarSize = Vector3(
						mazeScalarf * 1.5f,
						1.5f,
						mazeScalarf * 1.5f
					);

					if (clientRnodes[updateID])
					{
						clientRnodes[updateID]->SetTransform(mazeScalarMat4 * Matrix4::Translation(cellpos + cellsize * 0.5f) * Matrix4::Scale(avatarSize * 0.5f));
					}
				}
				else
				{
					std::cout << "\t Failed to process updated avatar position from server.\n";
					return;
				}
				break;
			}
			case PacketType::PACKET_UPDATE_AVATAR_IDX:
			{
				//Update the client's avatar index
				if (CommonUtils::isInteger(packetData))
				{
					//Update the avatar's index into the allNodes array and the avatar physics node position
					int idx = std::stoi(packetData);
					avatarIdx = idx;
				}
				else
				{
					std::cout << "Couldn't update avatar index. Data wasn't an integer.\n";
				}
				break;
			}
			case PacketType::PACKET_PARAMS_REQUEST:
			{
				SendMazeParamsPacket();
				break;
			}
			case PacketType::PACKET_CLIENT_ID:
			{
				clientID = std::stoi(packetData);
				break;
			}
			case PacketType::PACKET_CLIENT_CONNECT:
			{
				int id = std::stoi(packetData);
				if (id != clientID)
				{
					Vector3 avatarPos = Vector3((16.5f + id % 4) * 3.0f, 0.0f, (-4.0f + id / 4) * 3.0f) * mazeScalarf;
					clientRnodes[id]->SetTransform(mazeScalarMat4 * Matrix4::Translation(avatarPos + cellsize * 0.5f) * Matrix4::Scale(avatarSize * 0.5f));
					clientGameObjs[id]->SetRender(clientRnodes[id]);
				}
				break;
			}
			case PacketType::PACKET_CLIENT_DISCONNECT:
			{
				int id = std::stoi(packetData);
				if (id != clientID)
				{
					Vector3 avatarPos = Vector3((16.5f + id % 4) * 3.0f, 0.0f, (-4.0f + id / 4) * 3.0f) * mazeScalarf;
					clientRnodes[id]->SetTransform(mazeScalarMat4 * Matrix4::Translation(avatarPos + cellsize * 0.5f) * Matrix4::Scale(avatarSize * 0.5f));
					GraphicsPipeline::Instance()->RemoveRenderNode(clientGameObjs[id]->Render());
					clientGameObjs[id]->SetRenderNode(NULL);
				}
				break;
			}
			case PacketType::PACKET_A_STAR_NODES:
			{
				aStarNodes = std::stoi(packetData);
				break;
			}
			case PacketType::PACKET_STRING_PULLING_NODES:
			{
				stringPullingNodes = std::stoi(packetData);
				break;
			}
			default:
			{
				std::cout << "\t Failed to read packet from Server. Unknown packet type " << packetType << ".\n";
				break;
			}
		}
	}
	break;


	//Server has disconnected
	case ENET_EVENT_TYPE_DISCONNECT:
	{
		NCLDebug::Log(status_color3, "Network: Server has disconnected!");
	}
	break;
	}
}

void ClientScene::GenerateNewMaze()
{
	UpdateIsMove(false);

	//delete all render nodes if they aren't set to be deleted by DeleteAllGameObjects
	//if the avatar obj's RenderNode was set to NULL when the maze was generated
	//then the avatarRender RenderNode won't be deleted by DeleteAllGameObjects so delete it here
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientGameObjs[i])
		{
			if (clientGameObjs[i]->HasRender())
			{
				if (clientID >= 0)
				{
					clientRnodes[i] = NULL;
				}
			}
			else
			{
				if (clientID >= 0)
				{
					SAFE_DELETE(clientRnodes[i]);
				}
			}
		}
	}
	this->DeleteAllGameObjects(); //Cleanup objects


	//The maze is returned in a [0,0,0] - [1,1,1] cube (with edge walls outside) regardless of maze size,
	// so we need to scale it to whatever size we want
	mazeScalarMat4 = Matrix4::Scale(Vector3(5.0f, 5.0f / float(mazeSize), 5.0f)) * Matrix4::Translation(Vector3(-0.5f, 0.0f, -0.5f));

	mazeRenderer = new MazeRenderer(mazeGenerator, wallMesh);
	mazeRenderer->Render()->SetTransform(Matrix4::Translation(pos_maze) * mazeScalarMat4);
	this->AddGameObject(mazeRenderer);

	uint flatMazeSize = mazeSize * 3 - 1;
	mazeScalarf = 1.0f / (float)flatMazeSize;

	//Create Ground (..we still have some common ground to work off)
	GameObject* ground = CommonUtils::BuildCuboidObject(
		"Ground",
		Vector3(0.7f, -0.2f, -0.7f),
		Vector3(3.3f, 0.2f, 3.3),
		false,
		0.0f,
		false,
		false,
		Vector4(0.2f, 0.5f, 1.0f, 1.0f));

	this->AddGameObject(ground);

	GraphNode* start = mazeGenerator->GetStartNode();
	GraphNode* end = mazeGenerator->GetEndNode();


	avatarSize = Vector3(mazeScalarf * 1.5f, 1.5f, mazeScalarf * 1.5f);
	cellsize = Vector3(mazeScalarf * 2.0f, 1.0f, mazeScalarf * 2.0f);

	Vector3 cellpos = Vector3(start->_pos.x * 3.0f, 0.0f, start->_pos.y * 3.0f) * mazeScalarf;

	//if the client knows its ID (which it always should by the time a maze is generated)
	//then set up game objects and render nodes arrays
	if (clientID >= 0)
	{
		//clientsConnectedreate the game object for each client
		//This clients render node is added to the game object when the avatar is told to follow its path
		//Other client render nodes are added to their game objects when they are connected and told to follow their path
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			Vector3 avatarPos = Vector3((16.5f + i % 4) * 3.0f, 0.0f, (-4.0f + i / 4) * 3.0f) * mazeScalarf;
			if (i == clientID)
			{
				clientRnodes[i] = new RenderNode(wallMesh, Vector4(0.0f, 0.0f, 1.0f, 1.0f));
				clientRnodes[i]->SetTransform(mazeScalarMat4 * Matrix4::Translation(avatarPos + cellsize * 0.5f) * Matrix4::Scale(avatarSize * 0.5f));
				clientGameObjs[i] = new GameObject("avatar " + std::to_string(i), clientRnodes[i], NULL);
			}
			else
			{
				clientRnodes[i] = new RenderNode(wallMesh, Vector4(1.0f, 1.0f, 0.2f, 1.0f));
				clientRnodes[i]->SetTransform(mazeScalarMat4 * Matrix4::Translation(avatarPos + cellsize * 0.5f) * Matrix4::Scale(avatarSize * 0.5f));
				clientGameObjs[i] = new GameObject("avatar " + std::to_string(i), NULL, NULL);
			}
			this->AddGameObject(clientGameObjs[i]);
		}
	}

	startNode = new GameObject("startnode", new RenderNode(wallMesh, Vector4(0.0f, 1.0f, 0.0f, 0.4f)), NULL);
	startNode->Render()->SetTransform(mazeScalarMat4 * Matrix4::Translation(cellpos + cellsize * 0.5f) * Matrix4::Scale(cellsize * 0.5f));
	this->AddGameObject(startNode);

	cellpos = Vector3(end->_pos.x * 3.0f, 0.0f, end->_pos.y * 3.0f) * mazeScalarf;

	endNode = new GameObject("endnode", new RenderNode(wallMesh, Vector4(1.0f, 0.0f, 0.0f, 0.4f)), NULL);
	endNode->Render()->SetTransform(mazeScalarMat4 * Matrix4::Translation(cellpos + cellsize * 0.5f) * Matrix4::Scale(cellsize * 0.5f));
	this->AddGameObject(endNode);

	//Request a path 
	RequestPath();
}

void ClientScene::UpdateAvatarServerPosition()
{
	Packet avatarPacket(PacketType::PACKET_UPDATE_AVATAR_IDX);
	std::string data = std::to_string(avatarIdx);
	avatarPacket.SetData(data);
	SendPacketToServer(avatarPacket);
}

void ClientScene::SendPacketToServer(const Packet& packet)
{
	std::ostringstream oss;
	oss << packet.GetPacketType() << " " << packet.GetData();
	char* packetWhole = _strdup(oss.str().c_str());

	ENetPacket* enetPacket = enet_packet_create(packetWhole, strlen(packetWhole) + 1, 0);
	enet_peer_send(serverConnection, 0, enetPacket);
}

void ClientScene::HandleKeyboardInputs()
{
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_G))
	{
		SendMazeParamsPacket();
	}

	//Toggle the drawing of the path
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_H))
	{
		wantToDrawPath = !wantToDrawPath;
		drawPath = wantToDrawPath;
	}

	//Spawn and move avatar
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_J))
	{
		wantToMove = true;
		UpdateIsMove(wantToMove);
		avatarIdx = mazeGenerator->GetStartIdx();
		clientGameObjs[clientID]->SetRender(clientRnodes[clientID]);
	}

	//Move the avatar
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_K))
	{
		wantToMove = !wantToMove;
		UpdateIsMove(wantToMove);
	}

	//Toggle string pulling
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_L))
	{
		useStringPulling = !useStringPulling;
		Packet stringPullingPacket(PacketType::PACKET_USE_STRING_PULLING);
		stringPullingPacket.SetData(to_string((int)useStringPulling));
		SendPacketToServer(stringPullingPacket);

		//Change the path with the new variable
		//RequestPath();
	}

	//End node movement (CTRL + Arrow key)
	if (Window::GetKeyboard()->KeyHeld(KEYBOARD_CONTROL))
	{
		if (mazeGenerator->GetEndNode())
		{
			if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_UP) && mazeGenerator->GetEndIdx() > mazeSize - 1)
			{
				UpdateIsMove(false);
				mazeGenerator->SetEndIdx(mazeGenerator->GetEndIdx() - mazeSize);
				UpdateEndPosition();
				mazeGenerator->SetStartIdx(avatarIdx);
				UpdateStartPosition();
				RequestPath();
			}

			if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_DOWN) && mazeGenerator->GetEndIdx() < mazeSize * (mazeSize - 1))
			{
				UpdateIsMove(false);
				mazeGenerator->SetEndIdx(mazeGenerator->GetEndIdx() + mazeSize);
				UpdateEndPosition();
				mazeGenerator->SetStartIdx(avatarIdx);
				UpdateStartPosition();
				RequestPath();
			}

			if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_RIGHT) && mazeGenerator->GetEndIdx() % mazeSize != mazeSize - 1)
			{
				UpdateIsMove(false);
				mazeGenerator->SetEndIdx(mazeGenerator->GetEndIdx() + 1);
				UpdateEndPosition();
				mazeGenerator->SetStartIdx(avatarIdx);
				UpdateStartPosition();
				RequestPath();
			}

			if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_LEFT) && mazeGenerator->GetEndIdx() % mazeSize != 0)
			{
				UpdateIsMove(false);
				mazeGenerator->SetEndIdx(mazeGenerator->GetEndIdx() - 1);
				UpdateEndPosition();
				mazeGenerator->SetStartIdx(avatarIdx);
				UpdateStartPosition();
				RequestPath();
			}
		}
	}
	//Start node movement (Arrow key)
	else if (mazeGenerator->GetStartNode())
	{
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_UP) && mazeGenerator->GetStartIdx() > mazeSize - 1)
		{
			UpdateIsMove(false);
			//Update the start node position on the generator
			mazeGenerator->SetStartIdx(mazeGenerator->GetStartIdx() - mazeSize);
			//This is then used to update the GameObject's position
			UpdateStartPosition();
			avatarIdx = mazeGenerator->GetStartIdx();
			UpdateAvatarServerPosition();
			RequestPath();
		}

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_DOWN) && mazeGenerator->GetStartIdx() < mazeSize * (mazeSize - 1))
		{
			UpdateIsMove(false);
			mazeGenerator->SetStartIdx(mazeGenerator->GetStartIdx() + mazeSize);
			UpdateStartPosition();
			avatarIdx = mazeGenerator->GetStartIdx();
			UpdateAvatarServerPosition();
			RequestPath();
		}

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_RIGHT) && mazeGenerator->GetStartIdx() % mazeSize != mazeSize - 1)
		{
			UpdateIsMove(false);
			mazeGenerator->SetStartIdx(mazeGenerator->GetStartIdx() + 1);
			UpdateStartPosition();
			avatarIdx = mazeGenerator->GetStartIdx();
			UpdateAvatarServerPosition();
			RequestPath();
		}

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_LEFT) && mazeGenerator->GetStartIdx() % mazeSize != 0)
		{
			UpdateIsMove(false);
			mazeGenerator->SetStartIdx(mazeGenerator->GetStartIdx() - 1);
			UpdateStartPosition();
			avatarIdx = mazeGenerator->GetStartIdx();
			UpdateAvatarServerPosition();
			RequestPath();
		}
	}

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_1))
	{
		mazeSize = min(mazeSize + 1, MAX_MAZE_SIZE);
	}

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_2))
	{
		mazeSize = max(mazeSize - 1, 2);
	}

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_3))
	{
		mazeDensity = min(mazeDensity + 0.1f, 1.0f);
	}

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_4))
	{
		mazeDensity = max(mazeDensity - 0.1f, 0.0f);
	}
}

void ClientScene::UpdateStartPosition()
{
	mazeGenerator->SetStartNode(&mazeGenerator->GetAllNodesArr()[mazeGenerator->GetStartIdx()]);
	GraphNode* start = mazeGenerator->GetStartNode();

	Vector3 cellpos = Vector3(
		start->_pos.x * 3.0f,
		0.0f,
		start->_pos.y * 3.0f
	) * mazeScalarf;

	Vector3 cellsize = Vector3(
		mazeScalarf * 2.0f,
		1.0f,
		mazeScalarf * 2.0f
	);

	startNode->Render()->SetTransform(mazeScalarMat4 * Matrix4::Translation(cellpos + cellsize * 0.5f) * Matrix4::Scale(cellsize * 0.5f));
}

void ClientScene::UpdateEndPosition()
{
	mazeGenerator->SetEndNode(&mazeGenerator->GetAllNodesArr()[mazeGenerator->GetEndIdx()]);
	GraphNode* end = mazeGenerator->GetEndNode();

	Vector3 cellpos = Vector3(
		end->_pos.x * 3.0f,
		1.0f,
		end->_pos.y * 3.0f
	) * mazeScalarf;

	Vector3 cellsize = Vector3(
		mazeScalarf * 2.0f,
		1.0f,
		mazeScalarf * 2.0f
	);

	endNode->Render()->SetTransform(mazeScalarMat4 * Matrix4::Translation(cellpos + cellsize * 0.5f) * Matrix4::Scale(cellsize * 0.5f));
}

//Update the start and end points server side and request a path between them
void ClientScene::RequestPath()
{
	Packet pathRequestPacket(PACKET_PATH_REQUEST);
	std::string data = std::to_string(mazeGenerator->GetStartIdx()) + " " +
		to_string(mazeGenerator->GetEndIdx());
	pathRequestPacket.SetData(data);
	SendPacketToServer(pathRequestPacket);
}

void ClientScene::DrawPath(const std::vector<int>& finalPath, float lineWidth)
{
	if (finalPath.size() > 0 && sizeof(mazeGenerator->GetAllNodesArr()) / sizeof(GraphNode*) <= finalPath.size())
	{
		float grid_scalar = 1.0f / (float)mazeGenerator->GetSize();
		float col_factor = 1.0f / (float)finalPath.size();

		Matrix4 transform = mazeRenderer->Render()->GetWorldTransform();

		for (int i = 0; i < finalPath.size() - 1; ++i)
		{
			Vector3 start = transform * Vector3(
				(mazeGenerator->GetAllNodesArr()[finalPath[i]]._pos.x + 0.5f) * grid_scalar,
				0.1f,
				(mazeGenerator->GetAllNodesArr()[finalPath[i]]._pos.y + 0.5f) * grid_scalar);

			Vector3 end = transform * Vector3(
				(mazeGenerator->GetAllNodesArr()[finalPath[i + 1]]._pos.x + 0.5f) * grid_scalar,
				0.1f,
				(mazeGenerator->GetAllNodesArr()[finalPath[i + 1]]._pos.y + 0.5f) * grid_scalar);

			NCLDebug::DrawThickLine(start, end, lineWidth, CommonUtils::GenColor(0.0f + i * col_factor));
		}
	}
}

void ClientScene::SendMazeParamsPacket()
{
	//Stop drawing the path until a path packet is received
	drawPath = false;

	Packet paramsPacket(PACKET_MAZE_PARAMS);
	std::string data = std::to_string(mazeSize) + std::string(" ") + std::to_string(mazeDensity);
	paramsPacket.SetData(data);
	SendPacketToServer(paramsPacket);
}

void ClientScene::UpdateIsMove(const bool move)
{
	isMove = move;
	//Send the new move bool to the server
	Packet isMovePacket(PacketType::PACKET_IS_MOVE);
	isMovePacket.SetData(to_string(isMove));
	SendPacketToServer(isMovePacket);
}
