#include <nclgl\Window.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\SceneManager.h>
#include <nclgl\NCLDebug.h>
#include <nclgl\PerfTimer.h>



#include "Phy2_Integration.h"
#include "Phy3_Constraints.h"
#include "Phy4_ColDetection.h"
#include "Phy4_AiCallbacks.h"
#include "Phy5_ColManifolds.h"
#include "Phy6_ColResponseElasticity.h"
#include "Phy6_ColResponseFriction.h"
#include "Phy7_Solver.h"

#include <iomanip>
#include "Phy8_Empty.h"

PerfTimer timer_total, timer_physics;
float fps;

bool draw_debug = true;
bool draw_performance = false;

void Quit(bool error = false, const string &reason = "");

void Initialize()
{
	//Initialise the Window
	if (!Window::Initialise("Game Technologies - Collision Resolution", 1280, 800, false))
		Quit(true, "Window failed to initialise!");

	//Initialise the PhysicsEngine
	PhysicsEngine::Instance();

	//Initialize Renderer
	GraphicsPipeline::Instance();
	SceneManager::Instance();	//Loads CommonMeshes in here (So everything after this can use them globally e.g. our scenes)

								//Enqueue All Scenes
								// - Add any new scenes you want here =D
	//SceneManager::Instance()->EnqueueScene(new Phy2_Integration("Physics Tut #2 - Integration"));
	//SceneManager::Instance()->EnqueueScene(new Phy3_Constraints("Physics Tut #3 - Distance Constraints"));
	//SceneManager::Instance()->EnqueueScene(new Phy4_ColDetection("Physics Tut #4 - Collision Detection"));
	//SceneManager::Instance()->EnqueueScene(new Phy4_AiCallbacks("Physics Tut #4 - Collision Detection [Bonus]"));
	//SceneManager::Instance()->EnqueueScene(new Phy5_ColManifolds("Physics Tut #5 - Collision Manifolds"));
	SceneManager::Instance()->EnqueueScene(new Phy6_ColResponseElasticity("Physics Tut #6 - Collision Response [Elasticity]"));
	SceneManager::Instance()->EnqueueScene(new Phy6_ColResponseFriction("Physics Tut #6 - Collision Response [Friction]"));
	SceneManager::Instance()->EnqueueScene(new Phy7_Solver("Physics Tut #7 - Global Solver"));
	SceneManager::Instance()->EnqueueScene(new Phy8_Empty("Empty"));

	GraphicsPipeline::Instance()->SetVsyncEnabled(true);
}




//------------------------------------
//---------Default main loop----------
//------------------------------------
// With GameTech, everything is put into 
// little "Scene" class's which are self contained
// programs with their own game objects/logic.
//
// So everything you want to do in renderer/main.cpp
// should now be able to be done inside a class object.
//
// For an example on how to set up your test Scene's,
// see one of the PhyX_xxxx tutorial scenes. =]



void Quit(bool error, const string &reason) {
	//Release Singletons
	SceneManager::Release();
	GraphicsPipeline::Release();
	PhysicsEngine::Release();
	Window::Destroy();

	//Show console reason before exit
	if (error) {
		std::cout << reason << std::endl;
		system("PAUSE");
		exit(-1);
	}
}

void PrintStatusEntries()
{
	const Vector4 status_color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	const Vector4 status_color_debug = Vector4(1.0f, 0.6f, 1.0f, 1.0f);
	const Vector4 status_color_performance = Vector4(1.0f, 0.6f, 0.6f, 1.0f);

	NCLDebug::AddStatusEntry(status_color, "FPS: %5.2f", fps);
	NCLDebug::AddStatusEntry(status_color, "UPS: %5.2f", 1000.f / timer_total.GetAvg());
	NCLDebug::AddStatusEntry(status_color, "Camera Speed: %f [- +]", GraphicsPipeline::Instance()->GetCamera()->GetSpeed());
	NCLDebug::AddStatusEntry(status_color, "Vsync: %s [N]", GraphicsPipeline::Instance()->GetVsyncEnabled() ? "Enabled " : "Disabled ");
	NCLDebug::AddStatusEntry(status_color, "Use Octree        : %s [U]", (PhysicsEngine::Instance()->UsingOctrees()) ? "Enabled " : "Disabled");
	NCLDebug::AddStatusEntry(status_color, "Use SphereSphere  : %s [I]", (PhysicsEngine::Instance()->UsingSphereSphere()) ? "Enabled " : "Disabled");
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << GraphicsPipeline::Instance()->GetCamera()->GetPosition();
	std::string s = "Camera Position: " + oss.str();
	NCLDebug::AddStatusEntry(status_color, s);

	NCLDebug::AddStatusEntry(Vector4(1.0f, 1.0f, 1.0f, 1.0f), "Sphere Sphere Checks    : %d", PhysicsEngine::Instance()->GetNumSphereSphereChecks());
	NCLDebug::AddStatusEntry(Vector4(1.0f, 1.0f, 1.0f, 1.0f), "Broadphase pairs        : %d", PhysicsEngine::Instance()->GetBroadphaseColPairs().size());

	//Print Current Scene Name
	NCLDebug::AddStatusEntry(status_color, "[%d/%d]: %s ([T]/[Y] to cycle or [R] to reload)",
		SceneManager::Instance()->GetCurrentSceneIndex() + 1,
		SceneManager::Instance()->SceneCount(),
		SceneManager::Instance()->GetCurrentScene()->GetSceneName().c_str()
	);

	//Print Engine Options
	NCLDebug::AddStatusEntry(Vector4(0.8f, 1.0f, 0.8f, 1.0f), "    Physics: %s [P]", PhysicsEngine::Instance()->IsPaused() ? "Paused  " : "Enabled ");
	NCLDebug::AddStatusEntry(status_color_performance, "");

	//Physics Debug Drawing options
	uint drawFlags = PhysicsEngine::Instance()->GetDebugDrawFlags();
	NCLDebug::AddStatusEntry(status_color_debug, "--- Debug Draw  [G] ---");
	if (draw_debug)
	{		
		NCLDebug::AddStatusEntry(status_color_debug, "Constraints       : %s [Z] - Tut 3+", (drawFlags & DEBUGDRAW_FLAGS_CONSTRAINT) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Collision Normals : %s [X] - Tut 4", (drawFlags & DEBUGDRAW_FLAGS_COLLISIONNORMALS) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Collision Volumes : %s [C] - Tut 4+", (drawFlags & DEBUGDRAW_FLAGS_COLLISIONVOLUMES) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Manifolds         : %s [V] - Tut 5+", (drawFlags & DEBUGDRAW_FLAGS_MANIFOLD) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Draw Octree       : %s [O]", (drawFlags & DEBUGDRAW_FLAGS_OCTREE) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Bounding Radius   : %s [B]", (drawFlags & DEBUGDRAW_FLAGS_BOUNDINGRADIUS) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "");
	}

	//Physics performance measurements
	NCLDebug::AddStatusEntry(status_color_performance, "--- Performance [H] ---");
	if (draw_performance)
	{
		timer_total.PrintOutputToStatusEntry(status_color_performance, "Frame Total     :");
		timer_physics.PrintOutputToStatusEntry(status_color_performance, "Physics Total   :");
		PhysicsEngine::Instance()->PrintPerformanceTimers(status_color_performance);
		NCLDebug::AddStatusEntry(status_color_performance, "");
	}
}


void HandleKeyboardInputs()
{
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_P))
	{
		PhysicsEngine::Instance()->SetPaused(!PhysicsEngine::Instance()->IsPaused());
		NCLLOG("[MAIN] - Physics %s", PhysicsEngine::Instance()->IsPaused() ? "paused" : "resumed");
	}

	uint sceneIdx = SceneManager::Instance()->GetCurrentSceneIndex();
	uint sceneMax = SceneManager::Instance()->SceneCount();
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_Y))
		SceneManager::Instance()->JumpToScene((sceneIdx + 1) % sceneMax);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_T))
		SceneManager::Instance()->JumpToScene((sceneIdx == 0 ? sceneMax : sceneIdx) - 1);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_R))
		SceneManager::Instance()->JumpToScene(sceneIdx);


	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_G)) draw_debug = !draw_debug;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_H)) draw_performance = !draw_performance;

	uint drawFlags = PhysicsEngine::Instance()->GetDebugDrawFlags();
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_Z))
		drawFlags ^= DEBUGDRAW_FLAGS_CONSTRAINT;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_X))
		drawFlags ^= DEBUGDRAW_FLAGS_COLLISIONNORMALS;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_C))
		drawFlags ^= DEBUGDRAW_FLAGS_COLLISIONVOLUMES;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_V))
		drawFlags ^= DEBUGDRAW_FLAGS_MANIFOLD;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_O))
		drawFlags ^= DEBUGDRAW_FLAGS_OCTREE;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_B))
		drawFlags ^= DEBUGDRAW_FLAGS_BOUNDINGRADIUS;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_U))
		PhysicsEngine::Instance()->ToggleOctrees();

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_I))
		PhysicsEngine::Instance()->ToggleSphereSphere();

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_M))
	{
		GraphicsPipeline::Instance()->ResetCamera();
	}

	//Toggle vsync
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_N))
	{
		GraphicsPipeline::Instance()->SetVsyncEnabled(!GraphicsPipeline::Instance()->GetVsyncEnabled());
	}
	
	//Fire sphere
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_J))
	{
		float projectTileSpeed = 50.0f;

		//Set direction to camera direction
		Vector3 direction = Matrix4::Rotation(GraphicsPipeline::Instance()->GetCamera()->GetYaw(), Vector3(0, 1, 0))
			* Matrix4::Rotation(GraphicsPipeline::Instance()->GetCamera()->GetPitch(), Vector3(1, 0, 0)) * Vector3(0, 0, -1);


		Vector4 color = CommonUtils::GenColor(RAND(), 1.0f);
		GameObject* obj = CommonUtils::BuildSphereObject(
			"",
			Vector3(GraphicsPipeline::Instance()->GetCamera()->GetPosition()),
			0.5f,
			true,
			1/10.0f,
			true,
			true,
			color,
			false);
		obj->Physics()->SetFriction(0.5f);
		obj->Physics()->SetElasticity(0.5f);
		obj->Physics()->SetLinearVelocity(direction * projectTileSpeed);
		SceneManager::Instance()->GetCurrentScene()->AddGameObject(obj);
	}

	//Fire cube
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_K))
	{
		float projectTileSpeed = 50.0f;

		//Set direction to camera direction
		Vector3 direction = Matrix4::Rotation(GraphicsPipeline::Instance()->GetCamera()->GetYaw(), Vector3(0, 1, 0))
			* Matrix4::Rotation(GraphicsPipeline::Instance()->GetCamera()->GetPitch(), Vector3(1, 0, 0)) * Vector3(0, 0, -1);

		Vector4 color = CommonUtils::GenColor(RAND(), 1.0f);
		GameObject* obj = CommonUtils::BuildCuboidObject(
			"",
			Vector3(GraphicsPipeline::Instance()->GetCamera()->GetPosition()),
			Vector3(0.5f, 0.5f, 0.5f),
			true,
			1 / 10.0f,
			true,
			true,
			color,
			false,
			CommonMeshes::MeshType::PORTAL_CUBE);
		obj->Physics()->SetElasticity(0.1f);
		obj->Physics()->SetFriction(0.9f);
		obj->Physics()->SetLinearVelocity(direction * projectTileSpeed);

		SceneManager::Instance()->GetCurrentScene()->AddGameObject(obj);
	}

	PhysicsEngine::Instance()->SetDebugDrawFlags(drawFlags);
}

int main()
{
	int frameCount = 0;
	float fpsTimer = 0.0f;
	float totalDT = 0.0f;
	fps = 0.0f;

	//Initialize our Window, Physics, Scenes etc
	Initialize();

	Window::GetWindow().GetTimer()->GetTimedMS();

	//Create main game-loop
	while (Window::GetWindow().UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {
		//Start Timing
		float dt = Window::GetWindow().GetTimer()->GetTimedMS() * 0.001f;	//How many milliseconds since last update?
		totalDT += dt;
		fpsTimer += dt;
		timer_total.BeginTimingSection();


		//Handle Keyboard Inputs
		HandleKeyboardInputs();

		//Update Performance Timers (Show results every second)
		timer_total.UpdateRealElapsedTime(dt);
		timer_physics.UpdateRealElapsedTime(dt);


		//Update Scene
		SceneManager::Instance()->GetCurrentScene()->FireOnSceneUpdate(dt);

		//Update Physics
		timer_physics.BeginTimingSection();
		PhysicsEngine::Instance()->Update(dt);
		timer_physics.EndTimingSection();
		PhysicsEngine::Instance()->DebugRender();

		//Render Scene
		//Only render if totalDT > 1/60 of a second
		const float oneSixtieth = 1.0f / 60.0f;
		//if (totalDT > oneSixtieth)
		{
			totalDT -= oneSixtieth;
			GraphicsPipeline::Instance()->UpdateScene(dt);
			GraphicsPipeline::Instance()->RenderScene();				 //Finish Timing
		    //Print Status Entries
			PrintStatusEntries();

			//if 1 second has passed update the fps count
			if (fpsTimer > 1.0f)
			{
				fps = frameCount / fpsTimer;
				frameCount = 0;
				fpsTimer = 0.0f;
			}

			frameCount++;
		}

		//if (1000.0f / timer_total.GetAvg() > 60.0f)
		//{
		//	fps = 60.0f;
		//}
		//else
		//{
		//	fps = 1000.0f / timer_total.GetAvg();
		//}
		timer_total.EndTimingSection();
	}

	//Cleanup
	Quit();
	return 0;
}