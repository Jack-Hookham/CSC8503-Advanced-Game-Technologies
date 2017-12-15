#include "NewtonsCradleScene.h"

#include <ncltech\CuboidCollisionShape.h>
#include <nclgl\Vector4.h>
#include <ncltech\GraphicsPipeline.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>

NewtonsCradleScene::NewtonsCradleScene(const std::string& friendly_name)
	: Scene(friendly_name)
	, m_AccumTime(0.0f)
	, m_pPlayer(NULL)
{
}

NewtonsCradleScene::~NewtonsCradleScene()
{

}


void NewtonsCradleScene::OnInitializeScene()
{
	//Set the camera position
	GraphicsPipeline::Instance()->GetCamera()->SetPosition(Vector3(0.0f, 10.0f, -18.0f));
	GraphicsPipeline::Instance()->GetCamera()->SetYaw(180.0f);
	GraphicsPipeline::Instance()->GetCamera()->SetPitch(-20.f);

	m_AccumTime = 0.0f;

	//<--- SCENE CREATION --->
	//Create Ground
	this->AddGameObject(CommonUtils::BuildCuboidObject("Ground", Vector3(0.0f, -1.0f, 0.0f), Vector3(20.0f, 1.0f, 20.0f), true, 0.0f, true, false, Vector4(1.0f, 1.0f, 1.0f, 1.0f)));

	GameObject* beam = new GameObject("Beam", new RenderNode(), new PhysicsNode());
	beam->Physics()->SetPosition(Vector3(0.0f, 10.0f, 0.0f));
	beam->Physics()->SetInverseMass(0.0f);
	beam->Physics()->SetInverseInertia(CuboidCollisionShape(Vector3(10.0f, 1.0f, 1.0f)).BuildInverseInertia(0.0f));


	GameObject* sphere1 = CommonUtils::BuildSphereObject(
		"Sphere1",			
		Vector3(0.0f, 5.0f, 0.0f),				
		1.0f,			
		true,				
		10.f,				
		true,				
		false,				
		CommonUtils::GenColor(RAND()));
	sphere1->Physics()->SetElasticity(0.99f);
	this->AddGameObject(sphere1);

	GameObject* sphere2 = CommonUtils::BuildSphereObject(
		"Sphere2",
		Vector3(2.0f, 5.0f, 0.0f),
		1.0f,
		true,
		10.f,
		true,
		true,
		CommonUtils::GenColor(RAND()));
	sphere2->Physics()->SetElasticity(0.99f);
	this->AddGameObject(sphere2);

	GameObject* sphere3 = CommonUtils::BuildSphereObject(
		"Sphere3",
		Vector3(-2.0f, 5.0f, 0.0f),
		1.0f,
		true,
		10.f,
		true,
		true,
		CommonUtils::GenColor(RAND()));
	sphere3->Physics()->SetElasticity(0.99f);
	this->AddGameObject(sphere3);

	GameObject* sphere4 = CommonUtils::BuildSphereObject(
		"Sphere3",
		Vector3(4.0f, 5.0f, 0.0f),
		1.0f,
		true,
		10.f,
		true,
		true,
		CommonUtils::GenColor(RAND()));
	sphere4->Physics()->SetElasticity(0.99f);
	this->AddGameObject(sphere4);

	GameObject* sphere5 = CommonUtils::BuildSphereObject(
		"Sphere3",
		Vector3(-9.0f, 10.0f, 0.0f),
		1.0f,
		true,
		10.f,
		true,
		true,
		CommonUtils::GenColor(RAND()));
	sphere5->Physics()->SetElasticity(0.99f);
	this->AddGameObject(sphere5);

	PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
		sphere1->Physics(),												
		beam->Physics(),												
		sphere1->Physics()->GetPosition() + Vector3(0.0f, 0.0f, 0.0f),	
		beam->Physics()->GetPosition() + Vector3(0.0f, 0.0f, 0.0f)));	

	PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
		sphere2->Physics(),												
		beam->Physics(),												
		sphere2->Physics()->GetPosition() + Vector3(0.0f, 0.0f, 0.0f),
		beam->Physics()->GetPosition() + Vector3(2.0f, 0.0f, 0.0f)));	

	PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
		sphere3->Physics(),
		beam->Physics(),
		sphere3->Physics()->GetPosition() + Vector3(0.0f, 0.0f, 0.0f),
		beam->Physics()->GetPosition() + Vector3(-2.0f, 0.0f, 0.0f)));

	PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
		sphere4->Physics(),
		beam->Physics(),
		sphere4->Physics()->GetPosition() + Vector3(0.0f, 0.0f, 0.0f),
		beam->Physics()->GetPosition() + Vector3(4.0f, 0.0f, 0.0f)));

	PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
		sphere5->Physics(),
		beam->Physics(),
		sphere5->Physics()->GetPosition() + Vector3(0.0f, 0.0f, 0.0f),
		beam->Physics()->GetPosition() + Vector3(-4.0f, 0.0f, 0.0f)));
}

void NewtonsCradleScene::OnCleanupScene()
{
	//Just delete all created game objects 
	//  - this is the default command on any Scene instance so we don't really need to override this function here.
	Scene::OnCleanupScene();
}

void NewtonsCradleScene::OnUpdateScene(float dt)
{
	m_AccumTime += dt;
}