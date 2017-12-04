#include "Octree.h"

Octree::Octree()
{
	m_root = new Octant();
}

Octree::Octree(BoundingBox region, std::vector<PhysicsNode*>& pNodes)
{
	m_root = new Octant(region, pNodes);
}

Octree::~Octree()
{
	SAFE_DELETE(m_root)
}

void Octree::insertObject()
{

}

void Octree::removeObject()
{
}

void Octree::updateObjects(std::vector<PhysicsNode*>& pNodes)
{
	m_pNodes = pNodes;
}

void Octree::buildOctree()
{
	//Update the physics nodes in the root octant
	m_root->updateObjects(m_pNodes);
	//Divide the root octant into 8 octants and split the physics nodes between these octants based on their position
	//These octants will then continue to be recursively divided until each octant contains a maximum 
	//number of physics nodes
	m_root->divideOctant();
}

void Octree::updateOctree()
{
}

void Octree::debugDraw()
{
	m_root->debugDraw();
}
