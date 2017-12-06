#pragma once

#include <ncltech\Scene.h>
#include <ncltech\CommonMeshes.h>
#include <nclgl\Vector4.h>
#include <ncltech\GraphicsPipeline.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>

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

	GLuint m_targetTexture;
	Vector4 goodColour = Vector4(0.1f, 1.0f, 0.1f, 1.0f);
	int goodScore = 100;
	float targetTimers[NUM_TARGETS] = { 0.0f };
	bool targetsOn[NUM_TARGETS] = { true };		//True - Green target, False - Red target

	GameObject* targets[NUM_TARGETS];
};