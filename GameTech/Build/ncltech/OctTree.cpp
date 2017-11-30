#include "OctTree.h"

OctTree::OctTree()
{
}

OctTree::OctTree(Vector3 pos, Vector3 size, std::vector<PhysicsNode> physicsNodes, OctTree* parent)
{
	mPos = pos;
	mSize = size;
	mPhysicsNodes = physicsNodes;
	mParent = parent;
}

OctTree::~OctTree()
{
}
