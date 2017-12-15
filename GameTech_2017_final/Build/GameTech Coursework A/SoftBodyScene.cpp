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
	if (m_allianceTexture)
	{
		glDeleteTextures(1, &m_allianceTexture);
		m_allianceTexture = 0;
	}

	if (m_hordeTexture)
	{
		glDeleteTextures(1, &m_hordeTexture);
		m_hordeTexture = 0;
	}

	if (m_pirateTexture)
	{
		glDeleteTextures(1, &m_pirateTexture);
		m_pirateTexture = 0;
	}
}


void SoftBodyScene::OnInitializeScene()
{
	//Set the camera position
	GraphicsPipeline::Instance()->GetCamera()->SetPosition(Vector3(15.0f, 10.0f, -15.0f));
	GraphicsPipeline::Instance()->GetCamera()->SetYaw(140.f);
	GraphicsPipeline::Instance()->GetCamera()->SetPitch(-20.f);
	InitTextures();

	m_AccumTime = 0.0f;

	//<--- SCENE CREATION --->
	//Create Ground
	this->AddGameObject(CommonUtils::BuildCuboidObject("Ground", Vector3(0.0f, -1.0f, 0.0f), Vector3(20.0f, 1.0f, 20.0f), true, 0.0f, true, false, Vector4(1.0f, 1.0f, 1.0f, 1.0f)));

	SoftBody* softBody1 = new SoftBody(
		"Alliance",
		16,
		10,
		0.5f,
		Vector3(-5.0f, 10.0f, 0.0f),
		10.0f,
		true,
		true,
		1,
		m_allianceTexture);
	this->AddGameObjectExtended(softBody1->SoftObject());

	SoftBody* softBody2 = new SoftBody(
		"Horde",
		16,
		10,
		0.5f,
		Vector3(5.0f, 10.0f, 0.0f),
		10.0f,
		true,
		true,
		2,
		m_hordeTexture);
	this->AddGameObjectExtended(softBody2->SoftObject());

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
}

void SoftBodyScene::InitTextures()
{
	m_allianceTexture = SOIL_load_OGL_texture(TEXTUREDIR"AllianceEmblem.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (m_allianceTexture)
	{
		glBindTexture(GL_TEXTURE_2D, m_allianceTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	m_hordeTexture = SOIL_load_OGL_texture(TEXTUREDIR"HordeEmblem.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (m_hordeTexture)
	{
		glBindTexture(GL_TEXTURE_2D, m_hordeTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}
