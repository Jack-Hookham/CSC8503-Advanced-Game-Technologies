#include <nclgl\Window.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\SceneManager.h>
#include <nclgl\NCLDebug.h>
#include <nclgl\PerfTimer.h>

#include <cuda_gl_interop.h>
#include <cuda_runtime.h>

#include <iostream>
#include <sstream> 
#include <iomanip>

#include "SandboxScene.h"
#include "PyramidScene.h"
#include "ScoreScene.h"
#include "EmptyScene.h"
#include "BallPoolScene.h"
#include "ConstraintsScene.h"
#include "SoftBodyScene.h"
#include "Scene_CollisionHandling.h"

const Vector4 status_colour = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
const Vector4 status_colour_header = Vector4(0.8f, 0.9f, 1.0f, 1.0f);
const Vector4 status_color_debug = Vector4(1.0f, 0.6f, 1.0f, 1.0f);

bool draw_debug = false;
bool show_perf_metrics = false;
PerfTimer timer_total, timer_physics, timer_update, timer_render;
uint shadowCycleKey = 4;

void Quit(bool error = false, const string &reason = "");

void Initialize()
{
	//Initialise the Window
	if (!Window::Initialise("Game Technologies - Real Time Physics and GPU computation", 1280, 800, false))
		Quit(true, "Window failed to initialise!");

	//Initialise the PhysicsEngine
	PhysicsEngine::Instance();

	//Initialize Renderer
	GraphicsPipeline::Instance();

	SceneManager::Instance();	//Loads CommonMeshes in here (So everything after this can use them globally e.g. our scenes)

	//Enqueue All Scenes
	SceneManager::Instance()->EnqueueScene(new SoftBodyScene("GameTech #2 - Soft Body"));
	SceneManager::Instance()->EnqueueScene(new PyramidScene("GameTech #2 - Pyramid"));
	SceneManager::Instance()->EnqueueScene(new ScoreScene("GameTech #3 - Projectile Game"));
	SceneManager::Instance()->EnqueueScene(new BallPoolScene("GameTech #4 - Ball Pool"));
	SceneManager::Instance()->EnqueueScene(new Scene_CollisionHandling("GameTech #5 - Ball Pool GPU acceleration"));

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
	//Print Current Scene Name
	NCLDebug::AddStatusEntry(status_colour_header, "[%d/%d]: %s ([T]/[Y] to cycle or [R] to reload)",
		SceneManager::Instance()->GetCurrentSceneIndex() + 1,
		SceneManager::Instance()->SceneCount(),
		SceneManager::Instance()->GetCurrentScene()->GetSceneName().c_str()
	);

	timer_total.PrintOutputToStatusEntry(status_colour, "Frame Time: ");

	//Print Engine Options
	NCLDebug::AddStatusEntry(status_colour_header, "NCLTech Settings");
	NCLDebug::AddStatusEntry(status_colour, "     Physics Engine: %s (Press P to toggle)", PhysicsEngine::Instance()->IsPaused() ? "Paused  " : "Enabled ");
	NCLDebug::AddStatusEntry(status_colour, "     Monitor V-Sync: %s (Press L to toggle)", GraphicsPipeline::Instance()->GetVsyncEnabled() ? "Enabled " : "Disabled");
	NCLDebug::AddStatusEntry(status_colour, "     Camera Speed: %f [- +]", GraphicsPipeline::Instance()->GetCamera()->GetSpeed());
	NCLDebug::AddStatusEntry(status_colour, "     Use Octree        : %s [U]", (PhysicsEngine::Instance()->UsingOctrees()) ? "Enabled " : "Disabled");
	NCLDebug::AddStatusEntry(status_colour, "     Use SphereSphere  : %s [I]", (PhysicsEngine::Instance()->UsingSphereSphere()) ? "Enabled " : "Disabled");

	NCLDebug::AddStatusEntry(Vector4(1.0f, 1.0f, 1.0f, 1.0f), "Sphere Sphere Checks    : %d", PhysicsEngine::Instance()->GetNumSphereSphereChecks());
	NCLDebug::AddStatusEntry(Vector4(1.0f, 1.0f, 1.0f, 1.0f), "Broadphase pairs        : %d", PhysicsEngine::Instance()->GetBroadphaseColPairs().size());

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << GraphicsPipeline::Instance()->GetCamera()->GetPosition();
	std::string s = "Camera Position: " + oss.str();
	NCLDebug::AddStatusEntry(status_colour, s);

	//Print debug info
	uint drawFlags = PhysicsEngine::Instance()->GetDebugDrawFlags();
	NCLDebug::AddStatusEntry(status_color_debug, "--- Debug Info  [G] ---");
	if (draw_debug)
	{
		NCLDebug::AddStatusEntry(status_color_debug, "Constraints       : %s [Z]", (drawFlags & DEBUGDRAW_FLAGS_CONSTRAINT) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Collision Normals : %s [X]", (drawFlags & DEBUGDRAW_FLAGS_COLLISIONNORMALS) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Collision Volumes : %s [C]", (drawFlags & DEBUGDRAW_FLAGS_COLLISIONVOLUMES) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Manifolds         : %s [V]", (drawFlags & DEBUGDRAW_FLAGS_MANIFOLD) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Draw Octree       : %s [O]", (drawFlags & DEBUGDRAW_FLAGS_OCTREE) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Bounding Radius   : %s [B]", (drawFlags & DEBUGDRAW_FLAGS_BOUNDINGRADIUS) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "");
	}
	NCLDebug::AddStatusEntry(status_colour, "");

	//Print Performance Timers
	NCLDebug::AddStatusEntry(status_colour, "FPS: %5.2f  (Press H for %s info)", 1000.f / timer_total.GetAvg(), show_perf_metrics ? "less" : "more");
	if (show_perf_metrics)
	{
		timer_total.PrintOutputToStatusEntry(status_colour, "          Total Time     :");
		timer_update.PrintOutputToStatusEntry(status_colour, "          Scene Update   :");
		timer_physics.PrintOutputToStatusEntry(status_colour, "          Physics Update :");
		timer_render.PrintOutputToStatusEntry(status_colour, "          Render Scene   :");
	}
	NCLDebug::AddStatusEntry(status_colour, "");
}


void HandleKeyboardInputs()
{
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_P))
		PhysicsEngine::Instance()->SetPaused(!PhysicsEngine::Instance()->IsPaused());

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_L))
		GraphicsPipeline::Instance()->SetVsyncEnabled(!GraphicsPipeline::Instance()->GetVsyncEnabled());

	uint sceneIdx = SceneManager::Instance()->GetCurrentSceneIndex();
	uint sceneMax = SceneManager::Instance()->SceneCount();
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_Y))
		SceneManager::Instance()->JumpToScene((sceneIdx + 1) % sceneMax);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_T))
		SceneManager::Instance()->JumpToScene((sceneIdx == 0 ? sceneMax : sceneIdx) - 1);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_R))
		SceneManager::Instance()->JumpToScene(sceneIdx);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_G))
		draw_debug = !draw_debug;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_H))
		show_perf_metrics = !show_perf_metrics;

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

	//Fire sphere
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_J))
	{
		float projectTileSpeed = 30.0f;

		//Set direction to camera direction
		Vector3 direction = Matrix4::Rotation(GraphicsPipeline::Instance()->GetCamera()->GetYaw(), Vector3(0, 1, 0))
			* Matrix4::Rotation(GraphicsPipeline::Instance()->GetCamera()->GetPitch(), Vector3(1, 0, 0)) * Vector3(0, 0, -1);


		Vector4 color = CommonUtils::GenColor(RAND(), 1.0f);
		GameObject* obj = CommonUtils::BuildSphereObject(
			"",
			Vector3(GraphicsPipeline::Instance()->GetCamera()->GetPosition()),
			0.5f,
			true,
			1 / 10.0f,
			true,
			true,
			color,
			false);
		obj->Physics()->SetElasticity(0.1f);
		obj->Physics()->SetFriction(0.9f);
		obj->Physics()->SetLinearVelocity(direction * projectTileSpeed);
		SceneManager::Instance()->GetCurrentScene()->AddGameObject(obj);
		SceneManager::Instance()->GetCurrentScene()->AddBallToCPUList(obj);
	}

	//Fire cube
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_K))
	{
		float projectTileSpeed = 30.0f;

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
		obj->Physics()->SetElasticity(0.2f);
		obj->Physics()->SetFriction(0.9f);
		obj->Physics()->SetLinearVelocity(direction * projectTileSpeed);

		//Initial push??
		//cube->Physics()->SetLinearVelocity(Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, 1.0f), 20.0f).ToMatrix3() * Vector3(-1.f, 0.f, 0.f));
		SceneManager::Instance()->GetCurrentScene()->AddGameObject(obj);
		SceneManager::Instance()->GetCurrentScene()->AddBallToCPUList(obj);
	}

	PhysicsEngine::Instance()->SetDebugDrawFlags(drawFlags);
}



int main()
{
	//Initialize our Window, Physics, Scenes etc
	Initialize();

	Window::GetWindow().GetTimer()->GetTimedMS();

	//Create main game-loop
	while (Window::GetWindow().UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE))
	{
		//Start Timing
		float dt = Window::GetWindow().GetTimer()->GetTimedMS() * 0.001f;	//How many milliseconds since last update?
		timer_total.UpdateRealElapsedTime(dt);

		timer_total.BeginTimingSection();
		//Print Status Entries
		PrintStatusEntries();

		//Handle Keyboard Inputs
		HandleKeyboardInputs();

		//Update Scene
		SceneManager::Instance()->GetCurrentScene()->FireOnSceneUpdate(dt);

		//Update Physics
		PhysicsEngine::Instance()->Update(dt);
		PhysicsEngine::Instance()->DebugRender();

		//Render Scene

		GraphicsPipeline::Instance()->UpdateScene(dt);
		GraphicsPipeline::Instance()->RenderScene();
		timer_total.EndTimingSection();
	}

	//Cleanup
	Quit();
	return 0;
}