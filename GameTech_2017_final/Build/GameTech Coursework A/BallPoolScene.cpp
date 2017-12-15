#include "BallPoolScene.h"

#include <nclgl\Vector4.h>
#include <ncltech\GraphicsPipeline.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>

BallPoolScene::BallPoolScene(const std::string& friendly_name)
	: Scene(friendly_name)
	, m_AccumTime(0.0f)
	, m_pPlayer(NULL)
{
}

BallPoolScene::~BallPoolScene()
{

}

void BallPoolScene::OnInitializeScene()
{
	//Set the camera position
	GraphicsPipeline::Instance()->GetCamera()->SetPosition(Vector3(20.0f, 15.0f, -20.0f));
	GraphicsPipeline::Instance()->GetCamera()->SetYaw(140.f);
	GraphicsPipeline::Instance()->GetCamera()->SetPitch(-20.f);

	m_AccumTime = 0.0f;

	//<--- SCENE CREATION --->
	//Create Ground
	this->AddGameObject(CommonUtils::BuildCuboidObject(
		"Ground", 
		Vector3(0.0f, -1.0f, 0.0f),
		Vector3(POOL_X, 1.0f, POOL_Z), 
		true, 
		0.0f, 
		true, 
		false, 
		Vector4(0.5f, 0.5f, 0.5f, 1.0f)));
	
	//Walls
	this->AddGameObject(CommonUtils::BuildCuboidObject(
		"Wall1", 
		Vector3(POOL_X + 1.0f, 3.0f, 0.0f),
		Vector3(1.0f, POOL_Y, POOL_Z),
		true,
		0.0f,
		true, 
		false, 
		Vector4(0.5f, 0.5f, 0.5f, 1.0f)));

	this->AddGameObject(CommonUtils::BuildCuboidObject(
		"Wall2",
		Vector3(-POOL_X - 1.0f, 3.0f, 0.0f),
		Vector3(1.0f, POOL_Y, POOL_Z),
		true,
		0.0f,
		true,
		false,
		Vector4(0.5f, 0.5f, 0.5f, 1.0f)));

	this->AddGameObject(CommonUtils::BuildCuboidObject(
		"Wall3",
		Vector3(0.0f, 3.0f, POOL_Z + 1.0f),
		Vector3(POOL_X, POOL_Y, 1.0f),
		true,
		0.0f,
		true,
		false,
		Vector4(0.5f, 0.5f, 0.5f, 1.0f)));

	this->AddGameObject(CommonUtils::BuildCuboidObject(
		"Wall4",
		Vector3(0.0f, 3.0f, -POOL_Z - 1.0f),
		Vector3(POOL_X, POOL_Y, 1.0f),
		true,
		0.0f,
		true,
		false,
		Vector4(1.0f, 1.0f, 1.0f, 1.0f)));
	
	for (size_t i = 0; i < NUM_BALLS; ++i)
	{
		Vector3 position = Vector3(-POOL_X + 1.0f + RAND() * (POOL_X - 1.0f) * 2.0f, 
			POOL_Y + RAND() * POOL_Y + 1.0f * 2.0f, 
			-POOL_Z + 1.0f + RAND() * (POOL_Z - 1.0f) * 2.0f);
		Vector4 color = CommonUtils::GenColor(RAND(), 1.0f);
		GameObject* obj = CommonUtils::BuildSphereObject(
			"",
			position,
			0.5f,
			true,
			1 / 10.0f,
			true,
			true,
			color,
			false);
		obj->Physics()->SetElasticity(0.2f);
		obj->Physics()->SetFriction(0.9f);
		SceneManager::Instance()->GetCurrentScene()->AddGameObject(obj);
	}
}

void BallPoolScene::OnCleanupScene()
{
	//Just delete all created game objects 
	//  - this is the default command on any Scene instance so we don't really need to override this function here.
	Scene::OnCleanupScene();
}

void BallPoolScene::OnUpdateScene(float dt)
{
	m_AccumTime += dt;
}