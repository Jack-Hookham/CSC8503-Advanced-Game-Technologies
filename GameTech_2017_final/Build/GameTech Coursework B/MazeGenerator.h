#pragma once
#include <ncltech\GameObject.h>
#include <ncltech\Scene.h>
#include "SearchAlgorithm.h"



class MazeGenerator
{
public:
	MazeGenerator(); //Maze_density goes from 1 (single path to exit) to 0 (no walls at all)
	virtual ~MazeGenerator();

	void Generate(int size, float maze_density);

	//All points on the maze grid are connected in some shape or form
	// so any two nodes picked randomly /will/ have a path between them
	inline GraphNode* GetStartNode() const { return start; }
	inline GraphNode* GetEndNode()  const { return end; }
	inline uint GetSize() const { return size; }

	//Used as a hack for the MazeRenderer to generate the walls more effeciently
	inline GraphNode* GetAllNodesArr() { return allNodes; }
	inline GraphEdge* GetAllEdgesArr() { return allEdges; }

	inline void SetStartNode(GraphNode* start) { this->start = start; }
	inline void SetEndNode(GraphNode* end) { this->end = end; }

	inline void SetStartIdx(const int idx)
	{
		startIdx = idx;
		start = &allNodes[idx];
	}
	inline void SetEndIdx(const int idx)
	{
		endIdx = idx;
		end = &allNodes[idx];
	}

	inline const int GetStartIdx() { return startIdx; }
	inline const int GetEndIdx() { return endIdx; }

protected:
	void GetRandomStartEndNodes();

	void Initiate_Arrays();

	void Generate_Prims();
	void Generate_Sparse(float density);

	uint size;
	GraphNode *start, *end;
	int startIdx;
	int endIdx;

	GraphNode* allNodes;
	GraphEdge* allEdges;
};