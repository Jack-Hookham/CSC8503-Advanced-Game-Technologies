#pragma once

#include <ncltech\Scene.h>

class SandboxScene : public Scene
{
public:
	SandboxScene(const std::string& friendly_name);
	virtual ~SandboxScene();

	virtual void OnInitializeScene()	 override;
	virtual void OnCleanupScene()		 override;
	virtual void OnUpdateScene(float dt) override;

protected:
	float m_AccumTime;
	GameObject* m_pPlayer;
};