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
	mazeGenerator = new MazeGenerator();

	while (true)
	{
		float dt = timer.GetTimedMS() * 0.001f;

		//Handle All Incoming Packets and Send any enqued packets
		networkBase.ServiceNetwork(dt, [&](const ENetEvent& evnt)
		{
			//ENetPeer* peer = evnt.peer;
			//const enet_uint16 clientID = peer->incomingPeerID;

			switch (evnt.type)
			{
				case ENET_EVENT_TYPE_CONNECT:
				{
					std::cout << ("- New Client Connected\n");
					clients[evnt.peer->incomingPeerID] = new Client(evnt.peer);

					//If there is a maze then send it to the client
					if (mazeGenerator && mazeDataPacket && mazeParamsPacket)
					{
						if (mazeDataPacket->GetPacketType() == PacketType::PACKET_MAZE_DATA)
						{
							std::cout << "\t Sending maze data to Client " << evnt.peer->incomingPeerID << ".\n";
							SendPacketToClient(evnt.peer, *mazeParamsPacket);
							SendPacketToClient(evnt.peer, *mazeDataPacket);
						}
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
							GenerateMazeDataPacket(packetData, delim, clientID);
							if (mazeDataPacket->GetPacketType() == PacketType::PACKET_MAZE_DATA)
							{
								//Update the maze parameters for all clients
								SendPacketToClients(*mazeParamsPacket);
								//Update the maze structure for all clients
								SendPacketToClients(*mazeDataPacket);
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
							mazeGenerator->SetStartIdx(std::stoi(packetTokens[0]));
							mazeGenerator->SetEndIdx(std::stoi(packetTokens[1]));

							std::cout << "\t Server generating path between Start (" << mazeGenerator->GetStartNode()->_pos.x << ", " << mazeGenerator->GetStartNode()->_pos.y <<
								") and End (" << mazeGenerator->GetEndNode()->_pos.x << ", " << mazeGenerator->GetEndNode()->_pos.y << ").\n";

							searchAStar->FindBestPath(mazeGenerator->GetStartNode(), mazeGenerator->GetEndNode());
							const std::list<const GraphNode*> finalPath = searchAStar->GetFinalPath();

							Packet pathPacket(PACKET_PATH);

							//Add the final path nodes to the packet data

							std::vector<std::string> pathIndiciesVec;



							for (auto it = finalPath.begin(); it != finalPath.end(); ++it)
							{
								//pathIndiciesVec.push_back((*it)->GetIndex());
								pathIndiciesVec.push_back(FindNode(*it));
							}

							std::string pathIndiciesString;
							pathIndiciesString = std::accumulate(std::begin(pathIndiciesVec), std::end(pathIndiciesVec), pathIndiciesString);
							pathPacket.SetData(pathIndiciesString);
							SendPacketToClient(peer, pathPacket);

							break;
						}

						default:
						{
							std::cout << "\t Failed to read packet from Client " << clientID << ". Unknown packet type " << packetType << ".\n";
							break;
						}
					}

					SearchAStar*		search_as;

					enet_packet_destroy(evnt.packet);
					break;
				}
				case ENET_EVENT_TYPE_DISCONNECT:
				{
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
}

void Server::SendPacketToClient(ENetPeer* peer, const Packet& packet)
{
	std::ostringstream oss;
	oss << packet.GetPacketType() << " " << packet.GetData();
	char* packetWhole = _strdup(oss.str().c_str());

	ENetPacket* enetPacket = enet_packet_create(packetWhole, strlen(packetWhole) + 1, 0);
	enet_peer_send(peer, 0, enetPacket);
}

std::string Server::FindNode(const GraphNode* node)
{
	Vector3 posToFind = node->_pos;

	for (int j = 0; j < mazeGenerator->GetSize() * mazeGenerator->GetSize(); ++j)
	{
		if (mazeGenerator->GetAllNodesArr()[j]._pos == posToFind)
		{
			return to_string(j) + " ";
		}
	}
	return "-1 ";
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
	std::string data = to_string(mazeSize) + std::string(" ") + to_string(mazeDensity);
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
