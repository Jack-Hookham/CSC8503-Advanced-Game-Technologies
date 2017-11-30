#include "Octant.h"

Octant::Octant()
{
	mRegion = BoundingBox(Vector3(), Vector3());
	mParent = NULL;
}

Octant::Octant(BoundingBox region, std::vector<PhysicsNode> physicsNodes, Octant* parent)
{
	mRegion = region;
	mPhysicsNodes = physicsNodes;
	mParent = parent;
}

Octant::~Octant()
{
}
