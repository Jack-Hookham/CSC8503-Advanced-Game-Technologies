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

	//Pool half dims
	const float POOL_X = 15.0f;
	const float POOL_Y = 5.0f;
	const float POOL_Z = 15.0f;
	const int NUM_BALLS = 500;
};