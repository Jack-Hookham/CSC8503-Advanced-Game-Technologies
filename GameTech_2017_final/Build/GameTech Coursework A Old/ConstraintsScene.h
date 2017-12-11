#pragma once

#include <ncltech\Scene.h>

class ConstraintsScene : public Scene
{
public:
	ConstraintsScene(const std::string& friendly_name);
	virtual ~ConstraintsScene();

	virtual void OnInitializeScene()	 override;
	virtual void OnCleanupScene()		 override;
	virtual void OnUpdateScene(float dt) override;

protected:
	float m_AccumTime;
	GameObject* m_pPlayer;
};