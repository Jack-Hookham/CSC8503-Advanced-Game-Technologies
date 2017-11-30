#pragma once
#include "nclgl/Vector3.h"
#include "PhysicsNode.h"

#define numChildren 8

class OctTree
{
public:
	OctTree();
	OctTree(Vector3 pos, Vector3 size, std::vector<PhysicsNode> physicsNodes, OctTree* parent);
	~OctTree();

private:
	Vector3 mPos;
	Vector3 mSize;
	std::vector<PhysicsNode> mPhysicsNodes;

	OctTree* mParent = NULL;
	OctTree* mChildren[numChildren] = { NULL };

	const int MIN_SIZE = 1;
	const int MAX_OBJECTS = 1;
};
