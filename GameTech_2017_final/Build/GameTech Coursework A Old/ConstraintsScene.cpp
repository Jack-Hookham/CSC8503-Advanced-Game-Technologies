#include "ConstraintsScene.h"

#include <nclgl\Vector4.h>
#include <ncltech\GraphicsPipeline.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>

ConstraintsScene::ConstraintsScene(const std::string& friendly_name)
	: Scene(friendly_name)
	, m_AccumTime(0.0f)
	, m_pPlayer(NULL)
{
}

ConstraintsScene::~ConstraintsScene()
{

}


void ConstraintsScene::OnInitializeScene()
{
	//Set the camera position
	GraphicsPipeline::Instance()->GetCamera()->SetPosition(Vector3(15.0f, 10.0f, -15.0f));
	GraphicsPipeline::Instance()->GetCamera()->SetYaw(140.f);
	GraphicsPipeline::Instance()->GetCamera()->SetPitch(-20.f);

	m_AccumTime = 0.0f;

	//<--- SCENE CREATION --->
	//Create Ground
	this->AddGameObject(CommonUtils::BuildCuboidObject("Ground", Vector3(0.0f, -1.0f, 0.0f), Vector3(20.0f, 1.0f, 20.0f), true, 0.0f, true, false, Vector4(1.0f, 1.0f, 1.0f, 1.0f)));

	GameObject* support1 = CommonUtils::BuildCuboidObject(
		"Support1",
		Vector3(-7.f, 7.f, 5.0f),				
		Vector3(0.5f, 10.0f, 0.5f),				
		true,									
		0.0f,									
		true,
		false,									
		Vector4(0.5f, 0.4f, 0.4f, 1.0f));
	this->AddGameObject(support1);

	GameObject* support2 = CommonUtils::BuildCuboidObject(
		"Support2",
		Vector3(-7.f, 7.f, -5.0f),
		Vector3(0.5f, 10.0f, 0.5f),
		true,
		0.0f,
		true,
		false,
		Vector4(0.5f, 0.4f, 0.4f, 1.0f));
	this->AddGameObject(support2);

	GameObject* log1 = CommonUtils::BuildCuboidObject(
		"Log1",
		Vector3(7.f, 7.f, -5.0f),
		Vector3(0.5f, 0.5f, 0.5f),
		true,
		20.0f,
		true,
		true,
		Vector4(0.5f, 0.4f, 0.4f, 1.0f));
	this->AddGameObject(log1);

	PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
		support1->Physics(),												//Physics Object A
		log1->Physics(),													//Physics Object B
		support1->Physics()->GetPosition() + Vector3(1.0f, 0.0f, 0.0f),		//Attachment Position on Object A	-> Currently the far right edge
		log1->Physics()->GetPosition() + Vector3(-0.5f, -0.5f, -0.5f)));	//Attachment Position on Object B	-> Currently the far left edge 

	GameObject* support3 = CommonUtils::BuildCuboidObject(
		"Support3",
		Vector3(7.f, 7.f, 5.0f),
		Vector3(0.5f, 10.0f, 0.5f),
		true,
		0.0f,
		true,
		false,
		Vector4(0.5f, 0.4f, 0.4f, 1.0f));
	this->AddGameObject(support3);

	GameObject* support4 = CommonUtils::BuildCuboidObject(
		"Support4",
		Vector3(7.f, 7.f, -5.0f),
		Vector3(0.5f, 10.0f, 0.5f),
		true,
		0.0f,
		true,
		false,
		Vector4(0.5f, 0.4f, 0.4f, 1.0f));
	this->AddGameObject(support4);

	GameObject* log2 = CommonUtils::BuildCuboidObject(
		"Log2",
		Vector3(7.f, 7.f, -5.0f),
		Vector3(0.5f, 0.5f, 0.5f),
		true,
		20.0f,
		true,
		true,
		Vector4(0.5f, 0.4f, 0.4f, 1.0f));
	this->AddGameObject(log2);

	PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
		support3->Physics(),												//Physics Object A
		log2->Physics(),													//Physics Object B
		support3->Physics()->GetPosition() + Vector3(1.0f, 0.0f, 0.0f),		//Attachment Position on Object A	-> Currently the far right edge
		log2->Physics()->GetPosition() + Vector3(-0.5f, -0.5f, -0.5f)));	//Attachment Position on Object B	-> Currently the far left edge 
}

void ConstraintsScene::OnCleanupScene()
{
	//Just delete all created game objects 
	//  - this is the default command on any Scene instance so we don't really need to override this function here.
	Scene::OnCleanupScene();
}

void ConstraintsScene::OnUpdateScene(float dt)
{
	m_AccumTime += dt;

	// You can add status entries to the top left from anywhere in the program
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.2f, 0.2f, 1.0f), "Welcome to the Game Tech Framework!");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   You can move the black car with the arrow keys");

	// You can print text using 'printf' formatting
	bool donkeys = false;
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   The %s in this scene are dragable", donkeys ? "donkeys" : "cubes");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   - Left click to move");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   - Right click to rotate (They will be more spinnable after tutorial 2)");
}