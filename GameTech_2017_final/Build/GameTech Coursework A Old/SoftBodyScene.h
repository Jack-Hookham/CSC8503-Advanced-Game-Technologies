#pragma once

#include <ncltech\Scene.h>
#include <ncltech\SoftBody.h>

class SoftBodyScene : public Scene
{
public:
	SoftBodyScene(const std::string& friendly_name);
	virtual ~SoftBodyScene();

	virtual void OnInitializeScene()	 override;
	virtual void OnCleanupScene()		 override;
	virtual void OnUpdateScene(float dt) override;

	void InitTextures();

protected:
	float m_AccumTime;
	GLuint m_allianceTexture;
	GLuint m_hordeTexture;
	GLuint m_pirateTexture;
};