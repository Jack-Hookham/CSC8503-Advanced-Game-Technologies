#include "ScoreScene.h"

#include <nclgl\Vector4.h>
#include <ncltech\GraphicsPipeline.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>

ScoreScene::ScoreScene(const std::string& friendly_name)
	: Scene(friendly_name)
	, m_AccumTime(0.0f)
{
}

ScoreScene::~ScoreScene()
{
}

void ScoreScene::OnInitializeScene()
{
	//Set the camera position
	GraphicsPipeline::Instance()->GetCamera()->SetPosition(Vector3(0.0f, 5.0f, -1.0f));
	GraphicsPipeline::Instance()->GetCamera()->SetYaw(0.0f);
	GraphicsPipeline::Instance()->GetCamera()->SetPitch(-0.0f);

	m_AccumTime = 0.0f;

	//<--- SCENE CREATION --->
	//Set up targets
	float startX = -20.0f;
	float startY = 5.0f;
	float startZ = -30.0f;
	float incrementX = 40.0f / TARGET_ROWS;
	float incrementY = 20.0f / TARGET_COLUMNS;
	float incrementZ = 40.0f / TARGET_ROWS;


	for (size_t i = 0; i < NUM_TARGETS; ++i)
	{
		float xPos = startX + i / TARGET_COLUMNS * incrementX;
		float yPos = startY + i % TARGET_COLUMNS * incrementY;
		float zPos = startZ - ((i % TARGET_COLUMNS) * incrementZ);

		std::string targetID = "Target " + i;
		targets[i] = new TargetObj(
			targetID,
			Vector3(xPos, yPos, zPos),
			Vector3(1.0f, 1.0f, 1.0f),
			true,
			0.0f,
			true,
			false,
			goodColour,
			false,
			CommonMeshes::MeshType::TARGET_CUBE);

		targets[i]->SetScore(goodScore);
		targets[i]->Physics()->SetOnCollisionCallback(
			std::bind(&ScoreScene::TargetOnHitCallBack, this,
				targets[i],
				std::placeholders::_1,
				std::placeholders::_2));

		this->AddGameObject(targets[i]);
	}

	totalScore = 0;
}

void ScoreScene::OnCleanupScene()
{
	Scene::OnCleanupScene();
}

void ScoreScene::OnUpdateScene(float dt)
{
	m_AccumTime += dt;

	//Update target data
	UpdateTargetStates(dt);


	// You can print text using 'printf' formatting
	bool donkeys = false;
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   - Left click to move");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   - Right click to rotate (They will be more spinnable after tutorial 2)");

	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "Score: " + std::to_string(totalScore));
}

bool ScoreScene::TargetOnHitCallBack(TargetObj* target, PhysicsNode* self, PhysicsNode* collidingObject)
{
	target->SetScoreUpdating(true);

	return true;
}

void ScoreScene::UpdateTargetStates(float dt)
{
	for (size_t i = 0; i < NUM_TARGETS; ++i)
	{
		//Update the score if needed
		if (targets[i]->GetScoreUpdating())
		{
			if (targets[i]->GetUpdateTimer() < 1e-5f)
			{
				//only update score once per collision
				totalScore += targets[i]->GetScore();
			}

			targets[i]->UpdateUpdateTimer(dt);

			if (targets[i]->GetUpdateTimer() > 0.1f)
			{
				//Reset variables once a small amount of time passes to ensure one collision 
				//doesn't cause the score to update more than once
				targets[i]->SetScoreUpdating(false);
				targets[i]->ResetUpdateTimer();
			}

			if (targets[i]->GetTargetOn())
			{
				targets[i]->SetScore(badScore);
				targets[i]->Render()->SetColorRecursive(badColour);
				targets[i]->SetTargetOn(false);
			}
		}

		if (!targets[i]->GetTargetOn())
		{
			//if the target is off (red)
			if (targets[i]->GetTargetTimer() > 20.0f)
			{
				targets[i]->SetTargetOn(true);
				targets[i]->Render()->SetColorRecursive(goodColour);
				targets[i]->SetScore(goodScore);
				targets[i]->ResetTargetTimer();
			}

			targets[i]->UpdateTargetTimer(dt);
		}
	}
}
