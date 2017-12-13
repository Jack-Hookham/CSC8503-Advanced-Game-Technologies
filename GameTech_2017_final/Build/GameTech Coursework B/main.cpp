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

GameTimer timer;

void Win32_PrintAllAdapterIPAddresses();

void InitializeClient();
void RunClient();
void QuitClient(bool error = false, const string &reason = "");

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

int main(int arcg, char** argv)
{
	Server* server = new Server();

	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet.\n");
		return EXIT_FAILURE;
	}

	//Initialize Server on Port 1234, with a possible 32 clients connected at any time
	//If it fails to initialise then this is a client
	if (!server->Initialize(SERVER_PORT, MAX_CLIENTS))
	{
		fprintf(stderr, "An error occurred while trying to create an ENet server host.\n");
		myType = CLIENT;
	}
	//If the server initialises then this is a server
	else
	{
		myType = SERVER;
	}

	switch (myType)
	{
		case SERVER:
		{
			printf("Server Initiated\n");

			Win32_PrintAllAdapterIPAddresses();

			timer.GetTimedMS();
			server->RunServer();
			server->Release();
		}

		case CLIENT:
		{
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
	SceneManager::Instance()->EnqueueScene(new ClientScene("Maze Client"));
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