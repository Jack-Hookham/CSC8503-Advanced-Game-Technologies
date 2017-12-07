#pragma once

#include <ncltech\Scene.h>
#include <ncltech\CommonMeshes.h>
#include <nclgl\Vector4.h>
#include <ncltech\GraphicsPipeline.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>

#include "TargetObj.h"

#define TARGET_ROWS 9
#define TARGET_COLUMNS 5
#define NUM_TARGETS TARGET_ROWS * TARGET_COLUMNS
//#define NUM_TARGETS 9

class ScoreScene : public Scene
{
public:
	ScoreScene(const std::string& friendly_name);
	virtual ~ScoreScene();

	virtual void OnInitializeScene()	 override;
	virtual void OnCleanupScene()		 override;
	virtual void OnUpdateScene(float dt) override;

protected:
	//static bool TargetOnHitCallBack(PhysicsNode* self, PhysicsNode* collidingObject);
	bool TargetOnHitCallBack2(TargetObj* target, PhysicsNode* self, PhysicsNode* collidingObject);

	void UpdateTargetStates(float dt);

	float m_AccumTime;

	int totalScore = 0;
	int goodScore = 100;
	int badScore = -50;
	Vector4 goodColour = Vector4(0.1f, 1.0f, 0.1f, 1.0f);
	Vector4 badColour = Vector4(1.0f, 0.1f, 0.1f, 1.0f);

	TargetObj* targets[NUM_TARGETS];
};