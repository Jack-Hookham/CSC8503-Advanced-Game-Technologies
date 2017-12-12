#include "ClientScene.h"

const Vector3 status_color3 = Vector3(1.0f, 0.6f, 0.6f);
const Vector4 status_color = Vector4(status_color3.x, status_color3.y, status_color3.z, 1.0f);

ClientScene::ClientScene(const std::string& friendly_name)
	: Scene(friendly_name)
	, serverConnection(NULL)
	, generator(NULL)
	, maze(NULL)
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

	GraphicsPipeline::Instance()->GetCamera()->SetPosition(Vector3(-1.5, 15.0f, 1));
	GraphicsPipeline::Instance()->GetCamera()->SetPitch(-80);
	GraphicsPipeline::Instance()->GetCamera()->SetYaw(0);

	generator = new MazeGenerator();

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
}

void ClientScene::OnCleanupScene()
{
	Scene::OnCleanupScene();

				//Send one final packet telling the server we are disconnecting
				// - We are not waiting to resend this, so if it fails to arrive
				//   the server will have to wait until we time out naturally
	enet_peer_disconnect_now(serverConnection, 0);

	//Release network and all associated data/peer connections
	network.Release();
	serverConnection = NULL;
	SAFE_DELETE(wallMesh);
	SAFE_DELETE(generator);
	SAFE_DELETE(maze);
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

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << GraphicsPipeline::Instance()->GetCamera()->GetPosition();
	std::string s = "Camera Position: " + oss.str();
	NCLDebug::AddStatusEntry(status_color, s);
	NCLDebug::AddStatusEntry(status_color, "Network Traffic");
	NCLDebug::AddStatusEntry(status_color, "    Incoming: %5.2fKbps", network.m_IncomingKb);
	NCLDebug::AddStatusEntry(status_color, "    Outgoing: %5.2fKbps", network.m_OutgoingKb);		
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "--- Controls ---");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   [G] To generate a new maze", mazeSize);
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Grid Size : %2d ([1]/[2] to change)", mazeSize);
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Density : %2.0f percent ([3]/[4] to change)", mazeDensity * 100.f);

	HandleKeyboardInputs();
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
			msgPacket.AddDataSpaced("Hellooo!");
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
			case PACKET_BAD:
			{
				std::cout << "\t Failed to read packet from Server. Unknown packet type.\n";
				break;
			}
			case PACKET_MESSAGE:
			{
				//Print the message
				std::cout << "\t Server broadcast: " << packetData << "\n";
				break;
			}
			case PACKET_MAZE_PARAMS:
			{
				//Client shouldn't receive maze param packets
				std::cout << "\t Error: Received maze data packet from Server";
				break;
			}
			case PACKET_MAZE_DATA:
			{
				const uint dataLength = mazeSize * (mazeSize - 1) * 2;
				//Ensure that data string isn't longer than the required length
				packetData.resize(dataLength);
				std::cout << "Packet type: " << packetType << ", Packet data: " << packetData << "\n";

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
				generator->Generate(mazeSize, 0.0f);
				//Populate the wall data with the data from the server
				GraphEdge* allEdges = generator->GetAllEdgesArr();
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
			case PACKET_MOVE_START:
			{
				//generator->GetStartNode()->_pos += Vector3(0.0f, 0.0f, 5.0f);
				break;
			}
			default:
			{
				std::cout << "\t Failed to read packet from Server. Unknown packet type.\n";
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
	this->DeleteAllGameObjects(); //Cleanup old mazes

	//The maze is returned in a [0,0,0] - [1,1,1] cube (with edge walls outside) regardless of grid_size,
	// so we need to scale it to whatever size we want
	Matrix4 mazeScalarMat4 = Matrix4::Scale(Vector3(5.0f, 5.0f / float(mazeSize), 5.0f)) * Matrix4::Translation(Vector3(-0.5f, 0.0f, -0.5f));

	maze = new MazeRenderer(generator, wallMesh);
	maze->Render()->SetTransform(Matrix4::Translation(pos_maze) * mazeScalarMat4);
	this->AddGameObject(maze);

	//Create Ground (..we still have some common ground to work off)
	GameObject* ground = CommonUtils::BuildCuboidObject(
		"Ground",
		Vector3(0.0f, -1.0f, 0.0f),
		Vector3(20.0f, 1.0f, 20.0f),
		false,
		0.0f,
		false,
		false,
		Vector4(0.2f, 0.5f, 1.0f, 1.0f));

	this->AddGameObject(ground);

	GraphNode* start = generator->GetStartNode();
	GraphNode* end = generator->GetEndNode();

	uint flatMazeSize = mazeSize * 3 - 1;
	mazeScalarf = 1.0f / (float)flatMazeSize;

	Vector3 cellpos = Vector3(
		start->_pos.x * 3.0f,
		0.0f,
		start->_pos.y * 3
	) * mazeScalarf;
	Vector3 cellsize = Vector3(
		mazeScalarf * 2,
		1.0f,
		mazeScalarf * 2
	);

	startNode = new GameObject("startnode", new RenderNode(wallMesh, Vector4(0.0f, 1.0f, 0.0f, 1.0f)), NULL);
	startNode->Render()->SetTransform(mazeScalarMat4 * Matrix4::Translation(cellpos + cellsize * 0.5f) * Matrix4::Scale(cellsize * 0.5f));
	this->AddGameObject(startNode);

	cellpos = Vector3(
		end->_pos.x * 3.0f,
		1.0f,
		end->_pos.y * 3.0f
	) * mazeScalarf;
	endNode = new GameObject("endnode", new RenderNode(wallMesh, Vector4(1.0f, 0.0f, 0.0f, 1.0f)), NULL);
	endNode->Render()->SetTransform(mazeScalarMat4 * Matrix4::Translation(cellpos + cellsize * 0.5f) * Matrix4::Scale(cellsize * 0.5f));
	this->AddGameObject(endNode);

	//UpdateAStarPreset();
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
		//Packet msgPacket(PACKET_MESSAGE);
		//std::ostringstream ossMsg;
		//ossMsg << "Make me a maze! Maze Size: " << mazeSize << ", Maze Density: " << mazeSize;
		//msgPacket.AddDataSpaced(ossMsg.str());
		//SendPacketToServer(msgPacket);

		Packet paramsPacket(PACKET_MAZE_PARAMS);
		paramsPacket.AddDataSpaced(mazeSize);
		paramsPacket.AddDataSpaced(mazeDensity);
		SendPacketToServer(paramsPacket);
	}

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_H))
	{
		Packet routeRequestPacket(ROUTE_REQUEST_PACKET);
		SendPacketToServer(routeRequestPacket);
	}

	//End node movement (CTRL + Arrow key)
	if (Window::GetKeyboard()->KeyHeld(KEYBOARD_CONTROL))
	{
		if (generator->GetEndNode())
		{
			if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_UP))
			{
				endNode->Render()->SetTransform(Matrix4::Translation(Vector3(0.0f, 0.0f, -15.0f) * mazeScalarf) * endNode->Render()->GetTransform());
			}

			if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_DOWN))
			{
				endNode->Render()->SetTransform(Matrix4::Translation(Vector3(0.0f, 0.0f, 15.0f) * mazeScalarf) * endNode->Render()->GetTransform());
			}

			if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_RIGHT))
			{
				endNode->Render()->SetTransform(Matrix4::Translation(Vector3(15.0f, 0.0f, 0.0f) * mazeScalarf) * endNode->Render()->GetTransform());
			}

			if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_LEFT))
			{
				endNode->Render()->SetTransform(Matrix4::Translation(Vector3(-15.0f, 0.0f, 0.0f) * mazeScalarf) * endNode->Render()->GetTransform());
			}
		}
	}
	//Start node movement (Arrow key)
	else if (generator->GetStartNode())
	{
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_UP))
		{
			//Send a move start position request to the server
			//Packet moveStartReq(PACKET_MOVE_START);
			//moveStartReq.AddData(MOVEMENT_UP);
			//SendPacketToServer(moveStartReq);

			startNode->Render()->SetTransform(Matrix4::Translation(Vector3(0.0f, 0.0f, -15.0f) * mazeScalarf) * startNode->Render()->GetTransform());
		}

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_DOWN))
		{
			startNode->Render()->SetTransform(Matrix4::Translation(Vector3(0.0f, 0.0f, 15.0f) * mazeScalarf) * startNode->Render()->GetTransform());
		}

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_RIGHT))
		{
			startNode->Render()->SetTransform(Matrix4::Translation(Vector3(15.0f, 0.0f, 0.0f) * mazeScalarf) * startNode->Render()->GetTransform());
		}

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_LEFT))
		{
			startNode->Render()->SetTransform(Matrix4::Translation(Vector3(-15.0f, 0.0f, 0.0f) * mazeScalarf) * startNode->Render()->GetTransform());
		}
	}

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_1))
	{
		mazeSize++;
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
