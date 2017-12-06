#pragma once

#include <ncltech\Scene.h>

class ScoreScene : public Scene
{
public:
	ScoreScene(const std::string& friendly_name);
	virtual ~ScoreScene();

	virtual void OnInitializeScene()	 override;
	virtual void OnCleanupScene()		 override;
	virtual void OnUpdateScene(float dt) override;

protected:
	float m_AccumTime;
	GameObject* m_pPlayer;
};