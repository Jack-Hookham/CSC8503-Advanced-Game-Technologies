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

	float startX = -20.0f;
	float incrementWidth = 40.0f / TARGET_ROWS;
	float incrementHeight = 40.0f / TARGET_COLUMNS;

	for (size_t i = 0; i < NUM_TARGETS; ++i)
	{
		std::string targetID = "Target " + i;
		targets[i] = CommonUtils::BuildCuboidObject(
			targetID,
			Vector3(startX + i / TARGET_COLUMNS * incrementWidth, 10.0f + i % TARGET_COLUMNS* incrementHeight, -40.0f),
			Vector3(1.0f, 1.0f, 1.0f),
			true,
			0.0f,
			true,
			false,
			goodColour);
		targets[i]->SetScore(goodScore);
		targets[i]->Physics()->SetOnCollisionCallback(TargetOnHitCallBack);
		this->AddGameObject(targets[i]);
	}

	//Create the target texture
	m_targetTexture = SOIL_load_OGL_texture(
		TEXTUREDIR"target.tga",
		SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT);

	glBindTexture(GL_TEXTURE_2D, m_targetTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	//This overrides the default checkerboard texture, setting all cubes to have the target texture
	CommonMeshes::Cube()->SetTexture(m_targetTexture);
}

void ScoreScene::OnCleanupScene()
{
	//Reset the cube texture to the checkerboard texture for other scenes
	CommonMeshes::Cube()->SetTexture(CommonMeshes::CheckerboardTex());

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

bool ScoreScene::TargetOnHitCallBack(PhysicsNode* self, PhysicsNode* collidingObject)
{
	self->GetParent()->SetScoreUpdating(true);

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
			if (targets[i]->GetTargetTimer() > 5.0f)
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
