#pragma once

#include "Octant.h"

//Octree class to manage the world's octants
//The octree contains a single octant which can be split into another 8 octants as required

class Octree
{
public:
	Octree();
	Octree(BoundingBox region, std::vector<PhysicsNode*>& pNodes);
	~Octree();

	//Add and remove objects from the octree
	void insertObject();
	void removeObject(); 
	void updateObjects(std::vector<PhysicsNode*>& pNodes);

	void buildOctree();
	void updateOctree();

	void deubDraw();

	//inline void setPhysicsNodes(const std::vector<PhysicsNode> pNodes) { m_physicsNodes = pNodes; }
	//inline std::vector<PhysicsNode> getPhysicsNodes() const { return m_physicsNodes; }

private:
	Octant* m_root = NULL;							//The octree contains a single root octant
	std::vector<PhysicsNode*> m_pNodes = { NULL };	//All of the physics nodes in the tree that need to be put into octants
};
