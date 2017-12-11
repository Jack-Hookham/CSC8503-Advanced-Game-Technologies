#include <ncltech\Scene.h>
#include <nclgl\Vector4.h>
#include <ncltech\GraphicsPipeline.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>

#include "RenderNodeParticles.h"
#include "CudaCollidingParticles.cuh"

using namespace CommonUtils;

class Scene_CollisionHandling : public Scene
{
public:
	Scene_CollisionHandling(const std::string& friendly_name)
		: Scene(friendly_name)
	{
	}

	virtual ~Scene_CollisionHandling()
	{

	}


	virtual void OnInitializeScene() override
	{
		//<--- SCENE CREATION --->
		//Create Ground
		this->AddGameObject(BuildCuboidObject("Ground", Vector3(0.0f, -1.0f, 0.0f), Vector3(20.0f, 1.0f, 20.0f), true, 0.0f, true, false, Vector4(0.2f, 0.5f, 1.0f, 1.0f)));
		GameObject* sphere1 = BuildSphereObject("Sphere1",
			Vector3(0.0f, 10.0f, 0.0f),
			0.5f,
			true,
			1.0f,
			true,
			true,
			Vector4(1.0f, 0.5f, 0.5f, 1.0f));
		this->AddGameObject(sphere1);
		cpuBalls.push_back(sphere1);

		cudaParticleProg = new CudaCollidingParticles();

		//The dam size (<value> * PARTICLE_RADIUS * 2) must be smaller than the simulation world size!
		cudaParticleProg->InitializeParticleDam(32, 32, 32);

		uint num_particles = cudaParticleProg->GetNumParticles();

		RenderNodeParticles* rnode = new RenderNodeParticles();
		rnode->SetParticleRadius(PARTICLE_RADIUS);
		rnode->SetColor(Vector4(1.f, 0.f, 1.f, 1.f));
		rnode->GeneratePositionBuffer(num_particles, NULL);

		const float half_grid_world_size = PARTICLE_GRID_SIZE * PARTICLE_GRID_CELL_SIZE * 0.5f;
		rnode->SetTransform(Matrix4::Translation(Vector3(-half_grid_world_size, -half_grid_world_size, -half_grid_world_size)));

		//We don't need any game logic, or model matrices, just a means to render our
		// particles to the screen.. so this is just a wrapper  to our actual
		// vertex buffer that holds each particles world position.
		this->AddGameObject(new GameObject("", rnode, NULL));



		cudaParticleProg->InitializeOpenGLVertexBuffer(rnode->GetGLVertexBuffer());
	}

	virtual void OnCleanupScene() override
	{
		Scene::OnCleanupScene();
		delete cudaParticleProg;
	}

	virtual void OnUpdateScene(float dt) override
	{
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "--- Info ---");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "  Example broadphase using cuda thrust library. The thrust library");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "  is the GPU equivalent of the C++ STL and makes things easier ");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "  with vector's, sorting, iterators and array manipulation.");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "  No. Particles: %d", cudaParticleProg->GetNumParticles());

		int numBalls = cpuBalls.size();
		Vector3* position = new Vector3[numBalls];
		Vector3* velocity = new Vector3[numBalls];
		float* radius = new float[numBalls];
		for (int i = 0; i < numBalls; ++i)
		{
			if (cpuBalls[i]->HasPhysics())
			{
				position[i] = cpuBalls[i]->Physics()->GetPosition();
				velocity[i] = cpuBalls[i]->Physics()->GetLinearVelocity();
				radius[i] = cpuBalls[i]->Physics()->GetBoundingRadius();
			}
		}

		cudaParticleProg->UpdateParticles(dt, numBalls, position, velocity, radius);

		//SAFE_DELETE(position);
		//SAFE_DELETE(velocity);
		//SAFE_DELETE(radius);
	}


protected:
	CudaCollidingParticles* cudaParticleProg;
	std::vector<GameObject*> cpuBalls;
};