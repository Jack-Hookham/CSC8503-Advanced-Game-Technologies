#include "Octree.h"

Octree::Octree()
{
	m_region = BoundingBox();
	m_parent = NULL;
}

Octree::Octree(BoundingBox region, std::vector<PhysicsNode> physicsNodes, Octree* parent)
{
	m_region = region;
	m_physicsNodes = physicsNodes;
	m_parent = parent;
}

Octree::~Octree()
{
}

void Octree::insertObject()
{

}

void Octree::removeObject()
{
}

void Octree::buildOctree()
{
	//Only continue if the octree contains more than the maximum number of objects
	if (m_physicsNodes.size() <= MAX_OBJECTS)
	{
		return;
	}

	//Only continue if dimensions are greater than the minumum dimensions
	Vector3 dimensions = m_region._max - m_region._min;
	if (dimensions.x < MIN_SIZE && dimensions.y < MIN_SIZE && dimensions.z < MIN_SIZE)
	{
		return;
	}

	/*
	 *     +-------+
	 *	  /|      /|
	 *	 / |     / |
	 *	Y--|----+  |
	 *	|  Z----|--+
	 *	| /     | /
	 *	+-------X
	 */

	Vector3 halfDims = dimensions / 2;
	Vector3 centre = m_region._min + halfDims;
	//Divide the octree into its 8 octants
	BoundingBox octants[NUM_CHILDREN] = { BoundingBox() };
	octants[0] = BoundingBox(m_region._min, centre);		//bottom left close
	octants[1] = BoundingBox(Vector3(centre.x, m_region._min.y, m_region._min.z), Vector3());					//bottom right close
}

void Octree::updateOctree()
{
}
