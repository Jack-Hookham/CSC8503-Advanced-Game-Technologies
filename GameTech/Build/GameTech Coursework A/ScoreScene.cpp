#include "ScoreScene.h"

#include <nclgl\Vector4.h>
#include <ncltech\GraphicsPipeline.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>

GameObject* targets[NUM_TARGETS];

ScoreScene::ScoreScene(const std::string& friendly_name)
	: Scene(friendly_name)
	, m_AccumTime(0.0f)
	, m_pPlayer(NULL)
{
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
	float increment = 40.0f / NUM_TARGETS;

	Vector4 goodColour = Vector4(0.1f, 1.0f, 0.1f, 1.0f);

	for (size_t i = 0; i < NUM_TARGETS; ++i)
	{
		targets[i] = CommonUtils::BuildCuboidObject(
			"",
			Vector3(startX + i * increment, 10.0f, -40.0f),
			Vector3(1.0f, 1.0f, 1.0f),
			true,
			0.0f,
			true,
			false,
			goodColour);
		targets[i]->SetScore(100);
		targets[i]->Physics()->SetOnCollisionCallback(TargetOnHitCallBack);
		this->AddGameObject(targets[i]);
	}

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

	// You can print text using 'printf' formatting
	bool donkeys = false;
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   - Left click to move");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   - Right click to rotate (They will be more spinnable after tutorial 2)");
}

bool ScoreScene::TargetOnHitCallBack(PhysicsNode* self, PhysicsNode* collidingObject)
{

	self->GetParent()->Render()->SetColorRecursive(Vector4(1.0f, 0.1f, 0.1f, 1.0f));
	self->GetParent()->SetScore(-50);

	return true;
}
