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
	GraphicsPipeline::Instance()->GetCamera()->SetPosition(Vector3(15.0f, 10.0f, -15.0f));
	GraphicsPipeline::Instance()->GetCamera()->SetYaw(140.f);
	GraphicsPipeline::Instance()->GetCamera()->SetPitch(-20.f);

	m_AccumTime = 0.0f;

	//<--- SCENE CREATION --->
	//Create Ground
	this->AddGameObject(CommonUtils::BuildCuboidObject("Ground", Vector3(0.0f, -1.0f, 0.0f), Vector3(20.0f, 1.0f, 20.0f), true, 0.0f, true, false, Vector4(0.2f, 0.5f, 1.0f, 1.0f)));

	auto create_cube_tower = [&](const Vector3& offset, float cubewidth)
	{
		const Vector3 halfdims = Vector3(cubewidth, cubewidth, cubewidth) * 0.5f;
		for (int x = 0; x < 2; ++x)
		{
			for (int y = 0; y < 6; ++y)
			{
				uint idx = x * 5 + y;
				Vector4 color = CommonUtils::GenColor(idx / 10.f, 0.5f);
				Vector3 pos = offset + Vector3(x * cubewidth, 1e-3f + y * cubewidth, cubewidth * (idx % 2 == 0) ? 0.5f : -0.5f);

				GameObject* cube = CommonUtils::BuildCuboidObject(
					"",						// Optional: Name
					pos,					// Position
					halfdims,				// Half-Dimensions
					true,					// Physics Enabled?
					1.f,					// Physical Mass (must have physics enabled)
					true,					// Physically Collidable (has collision shape)
					true,					// Dragable by user?
					color);					// Render color
				this->AddGameObject(cube);
			}
		}
	};

	auto create_ball_cube = [&](const Vector3& offset, const Vector3& scale, float ballsize)
	{
		const int dims = 4;
		const Vector4 col = Vector4(1.0f, 0.5f, 0.2f, 1.0f);

		for (int x = 0; x < dims; ++x)
		{
			for (int y = 0; y < dims; ++y)
			{
				for (int z = 0; z < dims; ++z)
				{
					Vector3 pos = offset + Vector3(scale.x *x, scale.y * y, scale.z * z);
					GameObject* sphere = CommonUtils::BuildSphereObject(
						"",					// Optional: Name
						pos,				// Position
						ballsize,			// Half-Dimensions
						true,				// Physics Enabled?
						10.f,				// Physical Mass (must have physics enabled)
						true,				// Physically Collidable (has collision shape)
						false,				// Dragable by user?
						col);// Render color
					this->AddGameObject(sphere);
				}
			}
		}
	};

	//Create Cube Towers
	//create_cube_tower(Vector3(3.0f, 0.5f, 3.0f), 1.0f);
	//create_cube_tower(Vector3(-3.0f, 0.5f, -3.0f), 1.0f);

	////Create Test Ball Cubey things
	//create_ball_cube(Vector3(-8.0f, 0.5f, 12.0f), Vector3(0.5f, 0.5f, 0.5f), 0.1f);
	//create_ball_cube(Vector3(8.0f, 0.5f, 12.0f), Vector3(0.3f, 0.3f, 0.3f), 0.1f);
	//create_ball_cube(Vector3(-8.0f, 0.5f, -12.0f), Vector3(0.2f, 0.2f, 0.2f), 0.1f);
	//create_ball_cube(Vector3(8.0f, 0.5f, -12.0f), Vector3(0.5f, 0.5f, 0.5f), 0.1f);
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

	// You can add status entries to the top left from anywhere in the program
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.2f, 0.2f, 1.0f), "Welcome to the Game Tech Framework!");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   You can move the black car with the arrow keys");

	// You can print text using 'printf' formatting
	bool donkeys = false;
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   The %s in this scene are dragable", donkeys ? "donkeys" : "cubes");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   - Left click to move");
	NCLDebug::AddStatusEntry(Vector4(1.0f, 0.4f, 0.4f, 1.0f), "   - Right click to rotate (They will be more spinnable after tutorial 2)");
}