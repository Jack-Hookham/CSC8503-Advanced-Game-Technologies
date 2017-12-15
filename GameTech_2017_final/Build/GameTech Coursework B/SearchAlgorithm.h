#pragma once
#include <nclgl\Vector3.h>
#include <deque>
#include <list>
#include <map>

struct GraphEdge;

//A node in our graph that has a physical position and is linked to any number of other nodes via
// GraphEdge's. In this maze example were using a 2D grid, so graph nodes can only ever have a maximum
// of 4 neighbours. Though you can imagine that this could also be a 3D maze, or more realistically random
// points of interest in the world e.g. door, light switch, tv, couch for our AI to path between.
struct GraphNode
{
	//Small note: The order neighbours are listed is 'random'
	//  Lies! The maze actually sorts them at the end, to show the different algorithm patterns better. Though it shouldnt.
	std::deque<GraphEdge*> _neighbours;

	//Physical position of the node.
	// - for maze generator this is x,y grid cell
	// - for nav-mesh this is world space position of the node
	Vector3 _pos;

	//--- Used only by generation algorithm and debug drawing ---
	bool _visited;		//Just used for generation algorithm
};

//An edge that connects two graph nodes together
struct GraphEdge
{
	GraphNode* _a;
	GraphNode* _b;

	//Weight of the edge: e.g. how hard it is to traverse from _a to _b
	//1.0f for all maze generations, but worth noting. Maybe this edge goes over water and would take twice as long to traverse??
	float weighting;


	//--- Used only by generation algorithm and debug drawing ---
	bool _connected;
	bool _iswall;
};


typedef std::pair<const GraphNode*, const GraphNode*> SearchHistoryElement;
typedef std::list<SearchHistoryElement> SearchHistory;

typedef std::map<const GraphNode*, const GraphNode*> ParentMap;

class SearchAlgorithm
{
public:
	SearchAlgorithm() {	}
	virtual ~SearchAlgorithm() {}

	void StringPulling()
	{
		//No point string pulling if the path size is 2 or less
		if (finalPath.size() > 2)
		{
			//Store the new path nodes so that they can be added back into the final path at the end
			std::vector<const GraphNode*> newPath;

			//Add the first node to the path
			newPath.push_back(*(finalPath.begin()));

			//Set up previous node and next nodes
			auto currentNode = std::next(finalPath.begin());
			auto nextNode = std::next(currentNode);

			//Iterate through all the nodes in the old final path
			for (auto it = finalPath.begin(); it != finalPath.end() && nextNode != finalPath.end();)
			{
				Vector3 direction = Vector3();
				direction.x = (*it)->_pos.x - (*nextNode)->_pos.x;
				direction.z = (*it)->_pos.y - (*nextNode)->_pos.y;

				//The avatar only moves along the x and y axis (in maze space, this is translated to x and z in world space)
				float dot = Vector3::Dot(direction.Normalise(), Vector3(1, 0, 0));
				//https://stackoverflow.com/questions/15313808/how-to-check-if-float-is-a-whole-number
				float x = std::fabs(dot - std::round(dot));
				dot = std::fabs(dot - std::round(dot));
				if (dot > 0.000001f)
				{
					newPath.push_back(*currentNode);
					it = currentNode;
				}
				else
				{
					++currentNode;
					++nextNode;
				}
			}

			//Add the end position to the new path
			newPath.push_back(*std::prev(finalPath.end()));

			//Update the final path with the new path
			finalPath.clear();
			finalPath.assign(newPath.begin(), newPath.end());
		}
	}

	//Returns true/false on whether or not a path could be found
	virtual bool FindBestPath(const GraphNode* start, const GraphNode* goal) = 0;

	const SearchHistory&			   GetSearchHistory()	const { return searchHistory; }
	const std::list<const GraphNode*>& GetFinalPath()		const { return finalPath; }

protected:
	void BuildPathFromParentMap(const GraphNode* start, const GraphNode* goal, const ParentMap& map)
	{
		//To build a final path we can keep traversing back to the start node from the goal
		const GraphNode* current = goal;

		while (current != NULL)
		{
			finalPath.push_front(current);
			current = map.at(current);
		}
	}

protected:
	SearchHistory					searchHistory;
	std::list<const GraphNode*>     finalPath;
};