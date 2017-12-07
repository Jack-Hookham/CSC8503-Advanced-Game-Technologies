#pragma once

#include <ncltech\Scene.h>

class BallPoolScene : public Scene
{
public:
	BallPoolScene(const std::string& friendly_name);
	virtual ~BallPoolScene();

	virtual void OnInitializeScene()	 override;
	virtual void OnCleanupScene()		 override;
	virtual void OnUpdateScene(float dt) override;

protected:
	float m_AccumTime;
	GameObject* m_pPlayer;
};