#pragma once

#include <ncltech\Scene.h>

class EmptyScene : public Scene
{
public:
	EmptyScene(const std::string& friendly_name);
	virtual ~EmptyScene();

	virtual void OnInitializeScene()	 override;
	virtual void OnCleanupScene()		 override;
	virtual void OnUpdateScene(float dt) override;

protected:
	float m_AccumTime;
	GameObject* m_pPlayer;
};