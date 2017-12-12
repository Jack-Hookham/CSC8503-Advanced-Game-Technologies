#include "ClientScene.h"

const Vector3 status_color3 = Vector3(1.0f, 0.6f, 0.6f);
const Vector4 status_color = Vector4(status_color3.x, status_color3.y, status_color3.z, 1.0f);

ClientScene::ClientScene(const std::string& friendly_name)
	: Scene(friendly_name)
	, serverConnection(NULL)
{
	wallMesh = new OBJMesh(MESHDIR"cube.obj");

	GLuint whitetex;
	glGenTextures(1, &whitetex);
	glBindTexture(GL_TEXTURE_2D, whitetex);
	unsigned int pixel = 0xFFFFFFFF;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixel);
	glBindTexture(GL_TEXTURE_2D, 0);

	wallMesh->SetTexture(whitetex);
}

void ClientScene::OnInitializeScene()
{
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

	NCLDebug::AddStatusEntry(status_color, "Network Traffic");
	NCLDebug::AddStatusEntry(status_color, "    Incoming: %5.2fKbps", network.m_IncomingKb);
	NCLDebug::AddStatusEntry(status_color, "    Outgoing: %5.2fKbps", network.m_OutgoingKb);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_H))
	{
		Packet msgPacket(PACKET_MESSAGE);
		char* msg = "Hello";
		msgPacket.AddDataSpaced(msg);
		msg = "I am";
		msgPacket.AddDataSpaced(msg);
		msg = "client!";
		msgPacket.AddDataSpaced(msg);
		SendPacketToServer(msgPacket);
	}

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_G))
	{
		Packet msgPacket(PACKET_MESSAGE);
		std::ostringstream ossMsg;
		ossMsg << "Make me a maze! Maze Size: " << mazeSize << ", Maze Density: " << mazeSize;
		msgPacket.AddDataSpaced(ossMsg.str());
		SendPacketToServer(msgPacket);

		Packet paramsPacket(PACKET_MAZE_PARAMS);
		paramsPacket.AddDataSpaced(mazeSize);
		paramsPacket.AddDataSpaced(mazeDensity);
		SendPacketToServer(paramsPacket);
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
		std::cout << "Packet type: " << packetType << ", Packet data: " << packetData << "\n";

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
				//Process maze data packet, generate and render maze
				int length = packetData.length();
				bool* isWall = new bool[packetData.length()];
				int idx = 0;
				for (auto data : packetData)
				{
					isWall[idx] = (data == '1');
					idx++;
				}

				//Generate the maze from the packet data received from the client
				//Generate a maze with density 0
				generator->Generate(mazeSize, 0);
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
	Matrix4 maze_scalar = Matrix4::Scale(Vector3(5.f, 5.0f / float(mazeSize), 5.f)) * Matrix4::Translation(Vector3(-0.5f, 0.f, -0.5f));

	maze = new MazeRenderer(generator, wallMesh);
	maze->Render()->SetTransform(Matrix4::Translation(pos_maze) * maze_scalar);
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
	GraphNode* end = generator->GetGoalNode();

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
