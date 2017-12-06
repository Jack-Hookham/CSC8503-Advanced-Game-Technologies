#pragma once

#include <ncltech\Scene.h>
#include <ncltech\CommonMeshes.h>

#define NUM_TARGETS 10

class ScoreScene : public Scene
{
public:
	ScoreScene(const std::string& friendly_name);
	virtual ~ScoreScene();

	virtual void OnInitializeScene()	 override;
	virtual void OnCleanupScene()		 override;
	virtual void OnUpdateScene(float dt) override;

protected:
	static bool TargetOnHitCallBack(PhysicsNode* self, PhysicsNode* collidingObject);

	float m_AccumTime;
	GameObject* m_pPlayer;

	GLuint m_targetTexture;

	int totalScore = 0;
	float targetTimers[NUM_TARGETS] = { 0.0f };
	bool targetsOn[NUM_TARGETS] = { true };		//True - Green target, False - Red target
};