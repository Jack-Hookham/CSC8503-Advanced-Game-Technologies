#include "Server.h"
#include <numeric>

Server::Server()
	: generator(NULL)
	, searchAStar(new SearchAStar())
{
}

Server::~Server()
{
	networkBase.Release();

	SAFE_DELETE(generator);
	SAFE_DELETE(searchAStar);
}

void Server::RunServer()
{
	generator = new MazeGenerator();

	while (true)
	{
		float dt = timer.GetTimedMS() * 0.001f;

		//Handle All Incoming Packets and Send any enqued packets
		networkBase.ServiceNetwork(dt, [&](const ENetEvent& evnt)
		{
			switch (evnt.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
			{
				printf("- New Client Connected\n");
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE:
			{
				ENetPeer* peer = evnt.peer;
				const enet_uint16 clientID = peer->incomingPeerID;
				printf("\t Client %d packet received: %s\n", clientID, evnt.packet->data);

				std::string packetString((char*)evnt.packet->data);
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
						int mazeSize;
						std::string secondToken = packetData.substr(0, packetData.find(delim));
						packetData = packetData.substr(packetData.find_first_of(delim) + 1);
						if (CommonUtils::isInteger(secondToken))
						{
							mazeSize = std::stoi(secondToken);
						}
						else
						{
							mazeSize = 0;
							std::cout << "\t Failed to generate maze: Invalid maze size.\n";
							break;
						}

						float mazeDensity;
						std::string thirdToken = packetData.substr(0, packetData.find(delim));
						if (CommonUtils::isFloat(thirdToken))
						{
							mazeDensity = ::atof(thirdToken.c_str());
						}
						else
						{
							mazeDensity = 0.0f;
							std::cout << "\t Failed to generate maze: Invalid maze density.\n";
							break;
						}

						//Generate a maze with the given parameters and broadcast it to all clients
						std::cout << "\t Generating maze " << clientID << ": Generating maze. Maze Size: " << mazeSize << ", Maze Density: " << mazeDensity << "\n";
						generator->Generate(mazeSize, mazeDensity);

						GraphEdge* allEdges = generator->GetAllEdgesArr();
						Packet mazeData(PACKET_MAZE_DATA);			//Packet containing all of the maze wall information

						mazeData.InitData(new char[mazeSize * (mazeSize - 1) * 2]);

						uint base_offset = mazeSize * (mazeSize - 1);
						for (uint y = 0; y < mazeSize; ++y)
						{
							for (uint x = 0; x < mazeSize - 1; ++x)
							{
								GraphEdge* edgeX = &allEdges[(y * (mazeSize - 1) + x)];
								if (edgeX->_iswall)
								{
									mazeData.Data()[(y * (mazeSize - 1) + x)] = '1';
								}
								else
								{
									mazeData.Data()[(y * (mazeSize - 1) + x)] = '0';
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
									mazeData.Data()[base_offset + (x * (mazeSize - 1) + y)] = '1';
								}
								else
								{
									mazeData.Data()[base_offset + (x * (mazeSize - 1) + y)] = '0';
								}
							}
						}

						SendPacketToClients(mazeData);
						break;
					}
					case PacketType::PACKET_MAZE_DATA:
					{
						//Server shouldn't receive maze data packets
						std::cout << "\t Error: Received maze data packet from Client " << clientID << ".\n";
						break;
					}
					case PacketType::PATH_REQUEST_PACKET:
					{
						GraphNode* start = generator->GetStartNode();
						GraphNode* end = generator->GetEndNode();

						//Split the data into its (hopefully) 4 floats
						std::vector<std::string> packetTokens;
						std::stringstream ss(packetData);
						string token;
						while (getline(ss, token, delim))
						{
							packetTokens.push_back(token);
						}

						for (int i = 0; i < 4; ++i)
						{
							if (!CommonUtils::isFloat(packetTokens[i]))
							{
								packetType = PacketType::PACKET_BAD;
							}
						}
						if (packetType == PacketType::PACKET_BAD)
						{
							std::cout << "Failed to parse path request from Client " << clientID << ".\n";
							break;
						}

						//Set the new path start and end positions
						start->_pos.x = ::atof(packetTokens[0].c_str());
						start->_pos.y = ::atof(packetTokens[1].c_str());
						end->_pos.x = ::atof(packetTokens[2].c_str());
						end->_pos.y = ::atof(packetTokens[3].c_str());

						std::cout << "\t Server generating path between Start (" << generator->GetStartNode()->_pos.x << ", " << generator->GetStartNode()->_pos.y <<
							") and End (" << generator->GetEndNode()->_pos.x << ", " << generator->GetEndNode()->_pos.y << ").\n";

						searchAStar->FindBestPath(generator->GetStartNode(), generator->GetEndNode());
						const std::list<const GraphNode*> finalPath = searchAStar->GetFinalPath();

						Packet pathPacket(PATH_PACKET);

						//Add the final path nodes to the packet data
						int* pathIndices = new int[finalPath.size()];
						char* pathIndicesChar = new char[finalPath.size()];

						std::vector<std::string> pathIndiciesVec;

						for (auto it = finalPath.begin(); it != finalPath.end(); ++it)
						{
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

std::string Server::FindNode(const GraphNode * node)
{
	Vector3 posToFind = node->_pos;

	for (int j = 0; j < generator->GetSize() * generator->GetSize(); ++j)
	{
		if (generator->GetAllNodesArr()[j]._pos == posToFind)
		{
			return to_string(j) + " ";
		}
	}
	return "-1 ";
}
