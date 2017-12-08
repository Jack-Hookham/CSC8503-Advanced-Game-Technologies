
#pragma once

#include <ncltech\Scene.h>
#include <ncltech\NetworkBase.h>

class ClientScene : public Scene
{
public:
	ClientScene(const std::string& friendly_name);

	virtual void OnInitializeScene() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdateScene(float dt) override;

	void ProcessNetworkEvent(const ENetEvent& evnt);

protected:
	GameObject* box;


	NetworkBase network;
	ENetPeer*	serverConnection;
};