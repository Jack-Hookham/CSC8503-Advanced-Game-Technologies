#pragma once

#include <enet\enet.h>  //<-- MUST include this before "<nclgl\Window.h>"
#include <nclgl\GameTimer.h>
#include <nclgl\Vector3.h>
#include <nclgl\common.h>

#include <nclgl\Window.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\SceneManager.h>
#include <nclgl\NCLDebug.h>
#include <nclgl\PerfTimer.h>

#include "ClientScene.h"
#include "Server.h"
#include "Packet.h"
#include "MazeGenerator.h"
#include "SearchAStar.h"

//Needed to get computer adapter IPv4 addresses via windows
#include <iphlpapi.h>
#include "ncltech/CommonUtils.h"
#pragma comment(lib, "IPHLPAPI.lib")

#define SERVER_PORT 1234
#define UPDATE_TIMESTEP (1.0f / 30.0f) //send 30 position updates per second

NetworkBase server;
GameTimer timer;
float accum_time = 0.0f;
float rotation = 0.0f;

void Win32_PrintAllAdapterIPAddresses();

void InitializeClient();
void RunClient();
void RunServer();
void QuitClient(bool error = false, const string &reason = "");
void SendPacketToClients(const Packet& packet);

MazeGenerator* mazeGenerator = NULL;
SearchAStar* search_as;

enum Type
{
	SERVER,
	CLIENT
};

int myType = SERVER;

void PrintStatusEntries()
{
	const Vector4 status_color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	//Print Current Scene Name
	NCLDebug::AddStatusEntry(status_color, "[%d/%d]: %s ([T]/[Y] to cycle or [R] to reload)",
		SceneManager::Instance()->GetCurrentSceneIndex() + 1,
		SceneManager::Instance()->SceneCount(),
		SceneManager::Instance()->GetCurrentScene()->GetSceneName().c_str()
	);
}


void HandleKeyboardInputs()
{
	uint sceneIdx = SceneManager::Instance()->GetCurrentSceneIndex();
	uint sceneMax = SceneManager::Instance()->SceneCount();
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_Y))
		SceneManager::Instance()->JumpToScene((sceneIdx + 1) % sceneMax);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_T))
		SceneManager::Instance()->JumpToScene((sceneIdx == 0 ? sceneMax : sceneIdx) - 1);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_R))
		SceneManager::Instance()->JumpToScene(sceneIdx);
}

//Yay Win32 code >.>
//  - Grabs a list of all network adapters on the computer and prints out all IPv4 addresses associated with them.
void Win32_PrintAllAdapterIPAddresses()
{
	//Initially allocate 5KB of memory to store all adapter info
	ULONG outBufLen = 5000;


	IP_ADAPTER_INFO* pAdapters = NULL;
	DWORD status = ERROR_BUFFER_OVERFLOW;

	//Keep attempting to fit all adapter info inside our buffer, allocating more memory if needed
	// Note: Will exit after 5 failed attempts, or not enough memory. Lets pray it never comes to this!
	for (int i = 0; i < 5 && (status == ERROR_BUFFER_OVERFLOW); i++)
	{
		pAdapters = (IP_ADAPTER_INFO *)malloc(outBufLen);
		if (pAdapters != NULL) {

			//Get Network Adapter Info
			status = GetAdaptersInfo(pAdapters, &outBufLen);

			// Increase memory pool if needed
			if (status == ERROR_BUFFER_OVERFLOW) {
				free(pAdapters);
				pAdapters = NULL;
			}
			else {
				break;
			}
		}
	}


	if (pAdapters != NULL)
	{
		//Iterate through all Network Adapters, and print all IPv4 addresses associated with them to the console
		// - Adapters here are stored as a linked list termenated with a NULL next-pointer
		IP_ADAPTER_INFO* cAdapter = &pAdapters[0];
		while (cAdapter != NULL)
		{
			IP_ADDR_STRING* cIpAddress = &cAdapter->IpAddressList;
			while (cIpAddress != NULL)
			{
				printf("\t - Listening for connections on %s:%u\n", cIpAddress->IpAddress.String, SERVER_PORT);
				cIpAddress = cIpAddress->Next;
			}
			cAdapter = cAdapter->Next;
		}

		free(pAdapters);
	}
}


int onExit(int exitcode)
{
	server.Release();
	system("pause");
	exit(exitcode);
}

int main(int arcg, char** argv)
{
	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet.\n");
		return EXIT_FAILURE;
	}

	//Initialize Server on Port 1234, with a possible 32 clients connected at any time
	//If it fails to initialise then this is a client
	if (!server.Initialize(SERVER_PORT, 32))
	{
		fprintf(stderr, "An error occurred while trying to create an ENet server host.\n");
		//onExit(EXIT_FAILURE);
		myType = CLIENT;
	}
	//If the server initialises then this is a server
	else
	{
		//server = Server();
		myType = SERVER;
	}

	switch (myType)
	{
		case SERVER:
			printf("Server Initiated\n");

			Win32_PrintAllAdapterIPAddresses();

			timer.GetTimedMS();
			RunServer();

			system("pause");
			server.Release();

		case CLIENT:
			//--------------------------------------
			//---------Default client loop----------
			//--------------------------------------
			// With GameTech, everything is put into 
			// little "Scene" class's which are self contained
			// programs with their own game objects/logic.
			//
			// So everything you want to do in renderer/main.cpp
			// should now be able to be done inside a class object.
			//
			// For an example on how to set up your test Scene's,
			// see one of the PhyX_xxxx tutorial scenes. =]

			//Initialise our Window, Physics, Scenes etc
			InitializeClient();

			Window::GetWindow().GetTimer()->GetTimedMS();

			RunClient();

			//Cleanup
			QuitClient();
			return 0;
	}
}

void InitializeClient()
{
	//Initialise the Window
	if (!Window::Initialise("Game Technologies - Collision Resolution", 1280, 800, false))
		QuitClient(true, "Window failed to initialise!");

	//Initialise ENET for networking  //!!!!!!NEW!!!!!!!!
	if (enet_initialize() != 0)
	{
		QuitClient(true, "ENET failed to initialize!");
	}

	//Initialise the PhysicsEngine
	PhysicsEngine::Instance();

	//Initialize Renderer
	GraphicsPipeline::Instance();
	SceneManager::Instance();	//Loads CommonMeshes in here (So everything after this can use them globally e.g. our scenes)

								//Enqueue All Scenes
								// - Add any new scenes you want here =D
	SceneManager::Instance()->EnqueueScene(new ClientScene("Network #1 - Example Client"));
}

void QuitClient(bool error, const string &reason) {
	//Release Singletons
	SceneManager::Release();
	GraphicsPipeline::Release();
	PhysicsEngine::Release();
	enet_deinitialize();  //!!!!!!!!!!!!!!!!!NEW!!!!!!!!!!!!!!
	Window::Destroy();


	//Show console reason before exit
	if (error) {
		std::cout << reason << std::endl;
		system("PAUSE");
		exit(-1);
	}
}

void SendPacketToClients(const Packet& packet)
{
	std::ostringstream oss;
	oss << packet.GetPacketType() << " " << packet.GetData();
	char* packetWhole = _strdup(oss.str().c_str());

	ENetPacket* enetPacket = enet_packet_create(packetWhole, strlen(packetWhole) + 1, 0);
	enet_host_broadcast(server.m_pNetwork, 0, enetPacket);
}

void RunClient()
{
	//Main client-loop
	while (Window::GetWindow().UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {
		//Start Timing
		float dt = Window::GetWindow().GetTimer()->GetTimedMS() * 0.001f;	//How many milliseconds since last update?

		//Print Status Entries
		PrintStatusEntries();

		//Handle Keyboard Inputs
		HandleKeyboardInputs();

		//Update Scene
		SceneManager::Instance()->GetCurrentScene()->FireOnSceneUpdate(dt);

		//Update Physics
		PhysicsEngine::Instance()->Update(dt);
		PhysicsEngine::Instance()->DebugRender();

		//Render Scene

		GraphicsPipeline::Instance()->UpdateScene(dt);
		GraphicsPipeline::Instance()->RenderScene();				 //Finish Timing
	}
}

void RunServer()
{
	mazeGenerator = new MazeGenerator();

	while (true)
	{
		float dt = timer.GetTimedMS() * 0.001f;
		accum_time += dt;
		rotation += 0.5f * PI * dt;

		//Handle All Incoming Packets and Send any enqued packets
		server.ServiceNetwork(dt, [&](const ENetEvent& evnt)
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
					enet_uint16 clientID = evnt.peer->incomingPeerID;
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
							mazeGenerator->Generate(mazeSize, mazeDensity);

	

							GraphEdge* allEdges = mazeGenerator->GetAllEdgesArr();
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
										//mazeData.AddData('1');
										mazeData.Data()[(y * (mazeSize - 1) + x)] = '1';
									}
									else
									{
										//mazeData.AddData('0');
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
										//mazeData.AddData('1');
										mazeData.Data()[base_offset + (x * (mazeSize - 1) + y)] = '1';
									}
									else
									{
										//mazeData.AddData('0');
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
							printf("\t Error: Received maze data packet from Client %d", clientID);
							break;
						}
						default:
						{
							std::cout << "\t Failed to read packet from Client " << clientID << ". Unknown packet type.\n";
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

		//Broadcast update packet to all connected clients at a rate of UPDATE_TIMESTEP updates per second
		if (accum_time >= UPDATE_TIMESTEP)
		{
			//Packet data
			// - At the moment this is just a position update that rotates around the origin of the world
			//   though this can be any variable, structure or class you wish. Just remember that everything 
			//   you send takes up valuable network bandwidth so no sending every PhysicsObject struct each frame ;)
			accum_time = 0.0f;
			//Vector3 pos = Vector3(
			//	cos(rotation) * 2.0f,
			//	1.5f,
			//	sin(rotation) * 2.0f);

			////Create the packet and broadcast it (unreliable transport) to all clients
			//ENetPacket* position_update = enet_packet_create(&pos, sizeof(Vector3), 0);
			//enet_host_broadcast(server.m_pNetwork, 0, position_update);
		}

		Sleep(0);
	}

	SAFE_DELETE(mazeGenerator);
}