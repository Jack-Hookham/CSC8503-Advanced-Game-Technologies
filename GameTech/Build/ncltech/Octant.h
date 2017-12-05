#pragma once

#include "nclgl/Vector3.h"
#include "PhysicsNode.h"
#include "BoundingBox.h"
#include "CollisionDetectionSAT.h"
#include "nclgl/NCLDebug.h"

#define NUM_OCTANTS 8		//Number of child octants
#define MIN_SIZE 1			//Minumum dimensions for an octant
#define MAX_OBJECTS 5		//Maximum number of physics objects that can occupy an octant

class Octant
{
public:
	Octant();
	Octant(BoundingBox region, std::vector<PhysicsNode*>& pNodes, Octant* parent = NULL);
	~Octant();

	//Add and remove objects from the octree
	void insertObject();
	void removeObject();
	void updateObjects(std::vector<PhysicsNode*>& pNodes);

	void divideOctant();

	void genPairs(std::vector<CollisionPair>& colPairs);

	//Draw the outline of the octant and its child octants
	void debugDraw();

	Octant* getRoot();

	//inline void setPhysicsNodes(const std::vector<PhysicsNode> pNodes) { m_physicsNodes = pNodes; }
	inline std::vector<PhysicsNode*> getPhysicsNodes() const { return m_physicsNodes; }

private:
	BoundingBox m_region;							//The OctTree's bounding region
	std::vector<PhysicsNode*> m_physicsNodes;		//The physics objects contained within the OctTree
	Octant* m_parent = NULL;
	Octant* m_octants[NUM_OCTANTS] = { NULL };		//8 children
};