#pragma once

#include "nclgl/Vector3.h"
#include "PhysicsNode.h"

#define numChildren 8

//BoundingBox struct defines the bottom left and top right corners of the OctTree region
struct BoundingBox
{
	BoundingBox()
	{
		mBottomLeft = Vector3();
		mTopRight = Vector3();
	}

	BoundingBox(Vector3 bl, Vector3 tr)
	{
		mBottomLeft = bl;
		mTopRight = tr;
	}

	Vector3 mBottomLeft;
	Vector3 mTopRight;
};

//A single node in an OctTree
//Has a parent and up to 8 children
class Octant
{
	Octant();
	Octant(BoundingBox region, std::vector<PhysicsNode> physicsNodes, Octant* parent);
	~Octant();

	BoundingBox mRegion;						//The OctTree's bounding region
	std::vector<PhysicsNode> mPhysicsNodes;		//The physics objects contained within the OctTree

	Octant* mParent = NULL;
	Octant* mChildren[numChildren] = { NULL };	//8 children

	const int MIN_SIZE = 1;
	const int MAX_OBJECTS = 1;
};