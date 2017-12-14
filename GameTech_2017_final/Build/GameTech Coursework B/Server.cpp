#include "Server.h"
#include <numeric>

Server::Server()
	: mazeGenerator(NULL)
	, searchAStar(new SearchAStar())
	, mazeSize(0)
	, mazeDataPacket(NULL)
	, mazeParamsPacket(NULL)
	, clients{NULL}
{
}

Server::~Server()
{
	networkBase.Release();

	SAFE_DELETE(mazeGenerator);
	SAFE_DELETE(searchAStar);
	SAFE_DELETE(mazeDataPacket);
	SAFE_DELETE(mazeParamsPacket);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		SAFE_DELETE(clients[i]);
	}
}

void Server::RunServer()
{
	//Initialise the PhysicsEngine
	PhysicsEngine::Instance();
	mazeGenerator = new MazeGenerator();

	while (true)
	{
		float dt = timer.GetTimedMS() * 0.001f;
		//accumTime += dt;

		//Update the client physics nodes
		PhysicsEngine::Instance()->Update(dt);
		UpdateAvatars(dt);

		//Handle All Incoming Packets and Send any enqued packets
		networkBase.ServiceNetwork(dt, [&](const ENetEvent& evnt)
		{
			//ENetPeer* peer = evnt.peer;
			//const enet_uint16 clientID = peer->incomingPeerID;

			switch (evnt.type)
			{
				case ENET_EVENT_TYPE_CONNECT:
				{
					int clientID = evnt.peer->incomingPeerID;

					std::cout << ("- New Client Connected\n");
					clients[clientID] = new Client(evnt.peer);
					//Tell the new client what its ID is
					Packet idPacket(PacketType::PACKET_CLIENT_ID);
					idPacket.SetData(to_string(clientID));
					SendPacketToClient(evnt.peer, idPacket);

					//Send a packet to all clients telling them that a new client connected
					Packet clientConnectPacket(PacketType::PACKET_CLIENT_CONNECT);
					clientConnectPacket.SetData(to_string(clientID));
					SendPacketToClients(clientConnectPacket);

					//Add the client's physics node to the physics engine
					PhysicsEngine::Instance()->AddPhysicsObject(clients[clientID]->avatarPnode);

					//If there is a maze then send it to the client
					if (mazeGenerator && mazeDataPacket && mazeParamsPacket)
					{
						std::cout << "\t Sending maze data to Client " << clientID << ".\n";
						SendPacketToClient(evnt.peer, *mazeParamsPacket);
						SendPacketToClient(evnt.peer, *mazeDataPacket);

						//Tell the client about the other clients
						for (int i = 0; i < MAX_CLIENTS; ++i)
						{
							if (clients[i])
							{
								Packet clientPacket(PacketType::PACKET_CLIENT_CONNECT);
								clientPacket.SetData(to_string(i));
								SendPacketToClient(evnt.peer, clientPacket);
							}
						}
					}
					//if there is no maze then request maze parameters from the client
					else
					{
						Packet mazeParamsRequestPacket(PacketType::PACKET_PARAMS_REQUEST);
						//No data needed, just send the packet with the packet type
						mazeParamsRequestPacket.SetData("");
						SendPacketToClient(evnt.peer, mazeParamsRequestPacket);
					}
					break;
				}
				case ENET_EVENT_TYPE_RECEIVE:
				{
					enet_uint16 clientID = evnt.peer->incomingPeerID;
					ENetPeer* peer = clients[clientID]->peer;

					printf("\t Client %d packet received: %s\n", clientID, evnt.packet->data);

					std::string packetString((char*)evnt.packet->data);
					const char delim = ' ';

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

					switch (packetType)
					{
						case PacketType::PACKET_BAD:
						{
							std::cout << "\t Failed to read packet from Client " << clientID << ". Unknown packet type.\n";
							break;
						}
						case PacketType::PACKET_MESSAGE:
						{
							//Print the message
							std::cout << "\t Client " << clientID << " says: " << packetData << "\n";
							break;
						}
						case PacketType::PACKET_MAZE_PARAMS:
						{
							//Reset client information
							for (int i = 0; i < MAX_CLIENTS; ++i)
							{
								if (clients[i])
								{
									clients[i]->avatarIdx = 0;
									clients[i]->pathIdx = 0;
									clients[i]->avatarPosition = Vector2();
									clients[i]->pathTime = 0.0f;
									clients[i]->isMove = false;
									clients[i]->sendUpdateTime = 0.0f;
								}
							}

							GenerateMazeDataPacket(packetData, delim, clientID);
							if (mazeDataPacket->GetPacketType() == PacketType::PACKET_MAZE_DATA)
							{
								//Update the maze parameters for all clients
								SendPacketToClients(*mazeParamsPacket);
								//Update the maze structure for all clients
								SendPacketToClients(*mazeDataPacket);
							}

							//Tell all clients about each other
							for (int i = 0; i < MAX_CLIENTS; ++i)
							{
								if (clients[i])
								{
									Packet clientPacket(PacketType::PACKET_CLIENT_CONNECT);
									clientPacket.SetData(to_string(i));
									SendPacketToClients(clientPacket);
								}
							}

							break;
						}
						case PacketType::PACKET_MAZE_DATA:
						{
							//Server shouldn't receive maze data packets
							std::cout << "\t Error: Received maze data packet from Client " << clientID << ".\n";
							break;
						}
						case PacketType::PACKET_PATH_REQUEST:
						{
							clients[clientID]->pathIdx = 0;

							//Split the data into its (hopefully) 2 ints
							std::vector<std::string> packetTokens;
							std::stringstream ss(packetData);
							string token;
							while (getline(ss, token, delim))
							{
								packetTokens.push_back(token);
							}

							//Check that the data does contain the expected ints
							//If it doesn't then print and error message and stop processing the packet
							for (int i = 0; i < 2; ++i)
							{
								if (!CommonUtils::isInteger(packetTokens[i]))
								{
									packetType = PacketType::PACKET_BAD;
								}
							}
							if (packetType == PacketType::PACKET_BAD)
							{
								std::cout << "Failed to parse path request from Client " << clientID << ".\n";
								break;
							}

							//Set the new path start and end indicies from the client on the server
							int startIdx = std::stoi(packetTokens[0]);
							int endIdx = std::stoi(packetTokens[1]);

							//Update the start and end positions on the server
							clients[clientID]->startIdx = startIdx;
							clients[clientID]->endIdx = endIdx;

							std::cout << "\t Server generating path between Start (" << mazeGenerator->GetAllNodesArr()[startIdx]._pos.x <<
								", " << mazeGenerator->GetAllNodesArr()[startIdx]._pos.y << ") and End (" <<
								mazeGenerator->GetAllNodesArr()[endIdx]._pos.x << ", " <<
								mazeGenerator->GetAllNodesArr()[endIdx]._pos.y << ").\n";

							//Reset the avatar vars for the start node
							clients[clientID]->avatarPnode->SetPosition(Vector3(mazeGenerator->GetAllNodesArr()[startIdx]._pos.x, 0.0f, mazeGenerator->GetAllNodesArr()[startIdx]._pos.y));
							clients[clientID]->sendUpdateTime = 0.0f;
							clients[clientID]->pathTime = 0.0f;

							//Calculate A* path without string pulling
							searchAStar->FindBestPath(&mazeGenerator->GetAllNodesArr()[startIdx], &mazeGenerator->GetAllNodesArr()[endIdx]);
							std::list<const GraphNode*> finalPath = searchAStar->GetFinalPath();

							//Send the number of nodes produced by A* to the client so that it can be displayed for comparison
							Packet aStarPacket(PACKET_A_STAR_NODES);
							aStarPacket.SetData(to_string(finalPath.size()));
							SendPacketToClient(clients[clientID]->peer, aStarPacket);
							
							//Update the final path with string pulling to remove nodes between other nodes in LoS
							//searchAStar->StringPulling();
							finalPath = searchAStar->GetFinalPath();

							Packet stringPullPacket(PACKET_STRING_PULLING_NODES);
							stringPullPacket.SetData(to_string(finalPath.size()));
							SendPacketToClient(clients[clientID]->peer, stringPullPacket);

							Packet pathPacket(PACKET_PATH);

							//Add the final path nodes to the packet data
							std::vector<std::string> pathIndiciesStrings;
							//Also update the final path node indices for the current client
							clients[clientID]->pathIndices.clear();

							for (auto it = finalPath.begin(); it != finalPath.end(); ++it)
							{
								int idx = FindIdx(*it);
								pathIndiciesStrings.push_back(to_string(idx) + " ");
								clients[clientID]->pathIndices.push_back(idx);
							}

							std::string pathIndiciesString;
							pathIndiciesString = std::accumulate(std::begin(pathIndiciesStrings), std::end(pathIndiciesStrings), pathIndiciesString);
							pathPacket.SetData(pathIndiciesString);
							SendPacketToClient(peer, pathPacket);

							break;
						}
						//Update the avatar index on the server (happens when the client generates its start and end positions)
						case PacketType::PACKET_UPDATE_AVATAR_IDX:
						{
							//Update the current client's avatar index
							if (CommonUtils::isInteger(packetData))
							{
								//Update the avatar's index into the allNodes array and the avatar physics node position
								int idx = std::stoi(packetData);
								clients[clientID]->avatarIdx = idx;
								clients[clientID]->avatarPnode->SetPosition(Vector3(mazeGenerator->GetAllNodesArr()[idx]._pos.x, 0.0f, mazeGenerator->GetAllNodesArr()[idx]._pos.y));
							}
							else
							{
								std::cout << "Couldn't update avatar index for Client " << clientID << ". Data wasn't an integer.\n";
							}
							break;
						}
						//Update whether or not the client's avatar should move
						case PacketType::PACKET_IS_MOVE:
						{
							if (CommonUtils::isInteger(packetData))
							{
								std::istringstream(packetData) >> clients[clientID]->isMove;
							}
							else
							{
								std::cout << "Couldn't update movement bool for Client " << clientID << ". Data wasn't an integer.\n";
							}
						}

						default:
						{
							std::cout << "\t Failed to read packet from Client " << clientID << ". Unknown packet type " << packetType << ".\n";
							break;
						}
					}

					SearchAStar* search_as;

					enet_packet_destroy(evnt.packet);
					break;
				}
				case ENET_EVENT_TYPE_DISCONNECT:
				{
					//Send a packet to all clients telling them that the client disconnected
					Packet clientDisonnectPacket(PacketType::PACKET_CLIENT_DISCONNECT);
					clientDisonnectPacket.SetData(to_string(evnt.peer->incomingPeerID));
					SendPacketToClients(clientDisonnectPacket);

					PhysicsEngine::Instance()->RemovePhysicsObject(clients[evnt.peer->incomingPeerID]->avatarPnode);
					SAFE_DELETE(clients[evnt.peer->incomingPeerID]);
					printf("- Client %d has disconnected.\n", evnt.peer->incomingPeerID);
					break;
				}
			}
		});

		Sleep(0);
	}
}

void Server::SendPacketToClients(const Packet& packet)
{
	std::ostringstream oss;
	oss << packet.GetPacketType() << " " << packet.GetData();
	char* packetWhole = _strdup(oss.str().c_str());

	ENetPacket* enetPacket = enet_packet_create(packetWhole, strlen(packetWhole) + 1, 0);
	enet_host_broadcast(networkBase.m_pNetwork, 0, enetPacket);
	enet_host_flush(networkBase.m_pNetwork);
}

void Server::SendPacketToClient(ENetPeer* peer, const Packet& packet)
{
	std::ostringstream oss;
	oss << packet.GetPacketType() << " " << packet.GetData();
	char* packetWhole = _strdup(oss.str().c_str());

	ENetPacket* enetPacket = enet_packet_create(packetWhole, strlen(packetWhole) + 1, 0);
	enet_peer_send(peer, 0, enetPacket);
	enet_host_flush(networkBase.m_pNetwork);
}

const int Server::FindIdx(const GraphNode* node)
{
	Vector3 posToFind = node->_pos;

	for (int j = 0; j < mazeGenerator->GetSize() * mazeGenerator->GetSize(); ++j)
	{
		if (mazeGenerator->GetAllNodesArr()[j]._pos == posToFind)
		{
			//return std::to_string(j) + " ";
			return j;
		}
	}
	return -1;
}

void Server::GenerateMazeDataPacket(const std::string packetData, const char delim, const enet_uint16 clientID)
{
	//Delete the packet if it has been previously generated
	SAFE_DELETE(mazeDataPacket);
	SAFE_DELETE(mazeParamsPacket);

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
		mazeSize = std::stoi(packetTokens[0]);
		mazeDensity = std::atof(packetTokens[1].c_str());
	}
	else
	{
		mazeSize = 0;
		mazeDensity = 0.0f;
		std::cout << "\t Failed to generate maze: Invalid maze parameters.\n";
		mazeDataPacket = new Packet(PacketType::PACKET_BAD);
		mazeParamsPacket = new Packet(PacketType::PACKET_BAD);
		return;
	}

	//Generate a maze with the given parameters and broadcast it to all clients
	std::cout << "\t Generating maze " << clientID << ": Generating maze. Maze Size: " << mazeSize << ", Maze Density: " << mazeDensity << "\n";
	mazeGenerator->Generate(mazeSize, mazeDensity);

	mazeParamsPacket = new Packet(PACKET_MAZE_PARAMS);
	std::string data = std::to_string(mazeSize) + std::string(" ") + std::to_string(mazeDensity);
	mazeParamsPacket->SetData(data);

	GraphEdge* allEdges = mazeGenerator->GetAllEdgesArr();
	mazeDataPacket = new Packet(PACKET_MAZE_DATA);			//Packet containing all of the maze wall information

	//Allocate enough memory for the mazeSize, a space delimiter and all of the edge data
	mazeDataPacket->InitData(new char[mazeSize * (mazeSize - 1) * 2]);

	uint base_offset = mazeSize * (mazeSize - 1);
	for (uint y = 0; y < mazeSize; ++y)
	{
		for (uint x = 0; x < mazeSize - 1; ++x)
		{
			GraphEdge* edgeX = &allEdges[(y * (mazeSize - 1) + x)];
			if (edgeX->_iswall)
			{
				mazeDataPacket->Data()[(y * (mazeSize - 1) + x)] = '1';
			}
			else
			{
				mazeDataPacket->Data()[(y * (mazeSize - 1) + x)] = '0';
			}
		}
	}
	for (uint y = 0; y < mazeSize - 1; ++y)
	{
		for (uint x = 0; x < mazeSize; ++x)
		{
			GraphEdge* edgeY = &allEdges[base_offset + (x * (mazeSize - 1) + y)];
			if (edgeY->_iswall)
			{
				mazeDataPacket->Data()[base_offset + (x * (mazeSize - 1) + y)] = '1';
			}
			else
			{
				mazeDataPacket->Data()[base_offset + (x * (mazeSize - 1) + y)] = '0';
			}
		}
	}
}

void Server::UpdateAvatars(const float dt)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			UpdateAvatarVelocity(clients[i]);
			//if the client is connected and the client's movement bool is set then perform
			//movement logic/calculations
			if (clients[i]->isMove)
			{
				clients[i]->pathTime += dt;
				clients[i]->sendUpdateTime += dt;

				//Update the 
				if (clients[i]->pathTime > 1.0f / AVATAR_SPEED)
				{
					if (clients[i]->pathIdx < clients[i]->pathIndices.size() - 1)
					{
						clients[i]->pathIdx += (int)(clients[i]->pathTime * 1.0f / AVATAR_SPEED);
					}
					//Prevent the path index from going out of range
					if (clients[i]->pathIdx > clients[i]->pathIndices.size() - 1)
					{
						clients[i]->pathIdx = clients[i]->pathIndices.size() - 1;
					}

					clients[i]->avatarIdx = clients[i]->pathIndices[clients[i]->pathIdx];
					clients[i]->pathTime = std::fmod(clients[i]->pathTime, 1.0f / AVATAR_SPEED);
				}

				//Send client position updates every 1/30th of a second
				if (clients[i]->sendUpdateTime > UPDATE_TIMESTEP)
				{
					//Send client's physics node position to all clients
					Packet avatarPosPacket(PacketType::PACKET_UPDATE_AVATAR_POS);
					std::string data = std::to_string(i) + " " +
						std::to_string(clients[i]->avatarPnode->GetPosition().x) + " " +
						std::to_string(clients[i]->avatarPnode->GetPosition().z);
					avatarPosPacket.SetData(data);
					SendPacketToClients(avatarPosPacket);
					//Set new update time to remainder of update time / update time step
					clients[i]->sendUpdateTime = std::fmod(clients[i]->sendUpdateTime, UPDATE_TIMESTEP);

					//Send the each client's avatar position to them
					Packet avatarIndexPacket(PacketType::PACKET_UPDATE_AVATAR_IDX);
					avatarIndexPacket.SetData(std::to_string(clients[i]->avatarIdx));
					SendPacketToClient(clients[i]->peer, avatarIndexPacket);
				}
			}
		}
	}
}

void Server::UpdateAvatarVelocity(Client* client)
{
	if (!client->isMove)
	{
		client->avatarPnode->SetLinearVelocity(Vector3(0.0f, 0.0f, 0.0f));
	}
	else
	{
		GraphNode* currentNode = &mazeGenerator->GetAllNodesArr()[client->avatarIdx];
		GraphNode* nextNode;
		if (client->pathIdx < client->pathIndices.size() - 1)
		{
			nextNode = &mazeGenerator->GetAllNodesArr()[client->pathIndices[client->pathIdx + 1]];
		}
		else
		{
			nextNode = currentNode;
		}

		Vector3 direction = Vector3();
		direction.x = nextNode->_pos.x - currentNode->_pos.x;
		direction.z = nextNode->_pos.y - currentNode->_pos.y;

		client->avatarPnode->SetLinearVelocity(direction * AVATAR_SPEED);
	}
}