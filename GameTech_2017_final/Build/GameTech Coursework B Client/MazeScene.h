#pragma once

#include <ncltech\Scene.h>
#include "MazeGenerator.h"
#include "MazeRenderer.h"
#include "SearchAStar.h"
#include <ncltech\CommonUtils.h>
#include <nclgl\OBJMesh.h>
#include <deque>
#include <list>
#include <unordered_map>

const Vector3 pos_maze = Vector3(0.f, 0.f, 0.f);

//This class is mostly just debug/rendering chaff.
//The Maze graph is created via the MazeGenerator class, and is then
// traversed by one of the corresponding search algorithms. See SearchAStar.h
// of SearchBreadthFrist.h etc for actual implementation examples.
class MazeScene : public Scene
{
protected:
	MazeGenerator*	generator;
	MazeRenderer*	maze;

	SearchAStar*		search_as;

	int grid_size;
	float density;
	int astar_preset_idx;
	std::string astar_preset_text;

	Mesh* wallmesh;
public:
	void UpdateAStarPreset();

	MazeScene(const std::string& friendly_name);

	virtual ~MazeScene()
	{
		SAFE_DELETE(wallmesh);
	}



	virtual void OnInitializeScene() override
	{
		GraphicsPipeline::Instance()->GetCamera()->SetPosition(Vector3(-1.5, 25, 1));
		GraphicsPipeline::Instance()->GetCamera()->SetPitch(-80);
		GraphicsPipeline::Instance()->GetCamera()->SetYaw(0);

		GenerateNewMaze();
	}


	void GenerateNewMaze()
	{
		this->DeleteAllGameObjects(); //Cleanup old mazes

		generator->Generate(grid_size, density);

		//The maze is returned in a [0,0,0] - [1,1,1] cube (with edge walls outside) regardless of grid_size,
		// so we need to scale it to whatever size we want
		Matrix4 maze_scalar = Matrix4::Scale(Vector3(5.f, 5.0f / float(grid_size), 5.f)) * Matrix4::Translation(Vector3(-0.5f, 0.f, -0.5f));

		maze = new MazeRenderer(generator, wallmesh);
		maze->Render()->SetTransform(Matrix4::Translation(pos_maze) * maze_scalar);
		this->AddGameObject(maze);

		//Create Ground (..we still have some common ground to work off)
		GameObject* ground = CommonUtils::BuildCuboidObject(
			"Ground",
			Vector3(0.0f, -1.0f, 0.0f),
			Vector3(20.0f, 1.0f, 20.0f),
			false,
			0.0f,
			false,
			false,
			Vector4(0.2f, 0.5f, 1.0f, 1.0f));

		this->AddGameObject(ground);

		GraphNode* start = generator->GetStartNode();
		GraphNode* end = generator->GetGoalNode();

		UpdateAStarPreset();
	}



	virtual void OnUpdateScene(float dt) override
	{
		Scene::OnUpdateScene(dt);

		uint drawFlags = PhysicsEngine::Instance()->GetDebugDrawFlags();

		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "--- Controls ---");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   [G] To generate a new maze", grid_size);
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Grid Size : %2d ([1]/[2] to change)", grid_size);
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   Density : %2.0f percent ([3]/[4] to change)", density * 100.f);
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   A-Star Type: \"%s\" ([C] to cycle)", astar_preset_text.c_str());
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "----------------");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "Demonstration of path finding algorithms, and hopefully why A* is");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "so good at the job. See if you can figure out what the pros and cons");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "are of the various A* presets.");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "");

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_G))
		{
			GenerateNewMaze();
		}

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_1))
		{
			grid_size++;
			GenerateNewMaze();
		}

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_2))
		{
			grid_size = max(grid_size - 1, 2);
			GenerateNewMaze();
		}

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_3))
		{
			density = min(density + 0.1f, 1.0f);
			GenerateNewMaze();
		}

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_4))
		{
			density = max(density - 0.1f, 0.0f);
			GenerateNewMaze();
		}

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_C))
		{
			astar_preset_idx = (astar_preset_idx + 1) % 4;
			UpdateAStarPreset();
		}

		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "--- Number of nodes searched ---");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   A-Star       : %4d nodes", search_as->GetSearchHistory().size());

		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "--- Final path length ---");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "   A-Star       : %3d", search_as->GetFinalPath().size() - 1);

		NCLDebug::DrawTextWsNDT(pos_maze + Vector3(0, 0, 3.1f), STATUS_TEXT_SIZE, TEXTALIGN_CENTRE, Vector4(0, 0, 0, 1), "A-Star");

		maze->DrawSearchHistory(search_as->GetSearchHistory(), 2.5f / float(grid_size));
	}
};