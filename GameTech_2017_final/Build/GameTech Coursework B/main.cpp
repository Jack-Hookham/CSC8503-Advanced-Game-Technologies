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

//Needed to get computer adapter IPv4 addresses via windows
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#define SERVER_PORT 1234
#define UPDATE_TIMESTEP (1.0f / 30.0f) //send 30 position updates per second

NetworkBase server;
GameTimer timer;
float accum_time = 0.0f;
float rotation = 0.0f;

void Win32_PrintAllAdapterIPAddresses();

void InitializeClient();
void ClientLoop();
void ServerLoop();
void Quit(bool error = false, const string &reason = "");

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
			ServerLoop();

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

			ClientLoop();

			//Cleanup
			Quit();
			return 0;
	}
}

void InitializeClient()
{
	//Initialise the Window
	if (!Window::Initialise("Game Technologies - Collision Resolution", 1280, 800, false))
		Quit(true, "Window failed to initialise!");

	//Initialise ENET for networking  //!!!!!!NEW!!!!!!!!
	if (enet_initialize() != 0)
	{
		Quit(true, "ENET failed to initialize!");
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

void Quit(bool error, const string &reason) {
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

void ClientLoop()
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

void ServerLoop()
{
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
					printf("\t Client %d says: %s\n", evnt.peer->incomingPeerID, evnt.packet->data);

					//Get the first value in the packet to determine what to do with it
					//int packetType = evnt.packet->data[0];

					std::string packetData((char*)evnt.packet->data);
					enet_uint8 packetType = *evnt.packet->data;

					switch (packetType)
					{
					case PACKET_MESSAGE:
						{
							
						}
					case PACKET_MAZE_PARAMS:
						{
							
						}
					case PACKET_MAZE_DATA:
						{
							
						}
					}

					//printf
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
			Vector3 pos = Vector3(
				cos(rotation) * 2.0f,
				1.5f,
				sin(rotation) * 2.0f);

			//Create the packet and broadcast it (unreliable transport) to all clients
			ENetPacket* position_update = enet_packet_create(&pos, sizeof(Vector3), 0);
			enet_host_broadcast(server.m_pNetwork, 0, position_update);
		}

		Sleep(0);
	}
}