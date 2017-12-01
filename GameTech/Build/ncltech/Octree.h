#pragma once

#include "nclgl/Vector3.h"
#include "PhysicsNode.h"
#include "BoundingBox.h"

#define NUM_CHILDREN 8		//Number of child octrees in an octree
#define MIN_SIZE 1			//Minumum dimensions for an octree
#define MAX_OBJECTS 1		//Maximum number of physics objects that can occupy an octree segment

//OctTree class to manage the world's octants
//The octree is split into 8 octants which can each be split again into another 8 octants

class Octree
{
public:
	Octree();
	Octree(BoundingBox region, std::vector<PhysicsNode> physicsNodes, Octree* parent);
	~Octree();

	//Add and remove objects from the Octree
	void insertObject();
	void removeObject();

	void buildOctree();
	void updateOctree();

	inline void setPhysicsNodes(const std::vector<PhysicsNode> nodes) { m_physicsNodes = nodes; }
	inline std::vector<PhysicsNode> getPhysicsNodes() const { return m_physicsNodes; }

private:
	BoundingBox m_region;							//The OctTree's bounding region
	std::vector<PhysicsNode> m_physicsNodes;		//The physics objects contained within the OctTree
	Octree* m_parent = NULL;
	Octree* m_children[NUM_CHILDREN] = { NULL };	//8 children
};
