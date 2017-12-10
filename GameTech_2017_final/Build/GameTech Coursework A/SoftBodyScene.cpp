#include "SoftBodyScene.h"

#include <nclgl\Vector4.h>
#include <ncltech\GraphicsPipeline.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>

SoftBodyScene::SoftBodyScene(const std::string& friendly_name)
	: Scene(friendly_name)
	, m_AccumTime(0.0f)
{
}

SoftBodyScene::~SoftBodyScene()
{
}


void SoftBodyScene::OnInitializeScene()
{
	//Set the camera position
	GraphicsPipeline::Instance()->GetCamera()->SetPosition(Vector3(15.0f, 10.0f, -15.0f));
	GraphicsPipeline::Instance()->GetCamera()->SetYaw(140.f);
	GraphicsPipeline::Instance()->GetCamera()->SetPitch(-20.f);

	m_AccumTime = 0.0f;

	//<--- SCENE CREATION --->
	//Create Ground
	this->AddGameObject(CommonUtils::BuildCuboidObject("Ground", Vector3(0.0f, -1.0f, 0.0f), Vector3(20.0f, 1.0f, 20.0f), true, 0.0f, true, false, Vector4(1.0f, 1.0f, 1.0f, 1.0f)));

	SoftBody* softBody = new SoftBody(
		"Soft Body",
		20,
		20,
		0.5f,
		Vector3(0.0f, 10.0f, 0.0f),
		1.0f,
		true,
		true);

	this->AddGameObjectExtended(softBody->SoftObject());

	GraphicsPipeline::Instance()->GetCamera()->SetPosition(Vector3(5.0f, 15.0f, 20.0f));
	GraphicsPipeline::Instance()->GetCamera()->SetPitch(0.0f);
	GraphicsPipeline::Instance()->GetCamera()->SetYaw(0.0f);
}

void SoftBodyScene::OnCleanupScene()
{
	//Just delete all created game objects 
	//  - this is the default command on any Scene instance so we don't really need to override this function here.
	Scene::OnCleanupScene();
}

void SoftBodyScene::OnUpdateScene(float dt)
{
	m_AccumTime += dt;

	// You can print text using 'printf' formatting
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   - Left click to move");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   - Right click to rotate (They will be more spinnable after tutorial 2)");
}