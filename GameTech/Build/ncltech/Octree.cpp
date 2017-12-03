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
	//Divide the root octant into 8 octants
	//These octants will then continue to be recursively divided until each octant contains a maximum 
	//number of physics nodes

	m_root->updateObjects(m_pNodes);
	m_root->divideOctant();
}

void Octree::updateOctree()
{
}

void Octree::deubDraw()
{
	m_root->debugDraw();
}
