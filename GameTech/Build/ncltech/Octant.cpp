#include "Octant.h"
#include "nclgl/NCLDebug.h"

Octant::Octant()
{
	m_region = BoundingBox();
	m_parent = NULL;
}

Octant::Octant(BoundingBox region, std::vector<PhysicsNode*>& pNodes, Octant* parent)
{
	m_region = region;
	m_physicsNodes = pNodes;
	m_parent = parent;
}

Octant::~Octant()
{
	for (int i = 0; i < NUM_OCTANTS; ++i)
	{
		SAFE_DELETE(m_octants[i])
	}
}

void Octant::insertObject()
{

}

void Octant::removeObject()
{
}

void Octant::updateObjects(std::vector<PhysicsNode*>& pNodes)
{
	m_physicsNodes = pNodes;
}

void Octant::divideOctant()
{
	//Reset the octants before dividing
	for (int i = 0; i < NUM_OCTANTS; ++i)
	{
		delete m_octants[i];
		m_octants[i] = NULL;
	}

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
	*      +-------+
	*	  /|      /|
	*	 / |     / |
	*	Y--|----+  |
	*	|  Z----|--+
	*	| /     | /
	*	+-------X
	*/

	Vector3 halfDims = dimensions * 0.5f;
	Vector3 centre = m_region._min + halfDims;
	//Divide the octree into its 8 octants
	BoundingBox regions[NUM_OCTANTS] = { BoundingBox() };
	std::vector<PhysicsNode*> pNodeLists[NUM_OCTANTS];

	for (int i = 0; i < NUM_OCTANTS; ++i)
	{
		//Bitwise AND used to determine octant centres within the for loop
		Vector3 newCentre = centre;
		newCentre.x += halfDims.x * (i & 4 ? 0.5f : -0.5f);			//True for i = 4, 5, 6, 7 
		newCentre.y += halfDims.y * (i & 2 ? 0.5f : -0.5f);			//True for i = 2, 3, 6, 7
		newCentre.z += halfDims.z * (i & 1 ? 0.5f : -0.5f);			//True for i = 1, 3, 5, 7

		regions[i]._min = newCentre - halfDims * 0.5f;
		regions[i]._max = newCentre + halfDims * 0.5f;

		//Add the physicsNodes to each new octant if any point on the node's bounding radius is inside the octant
		for (PhysicsNode* pNode : m_physicsNodes)
		{
			if (pNode->GetPosition().x + pNode->GetBoundingRadius() > regions[i]._min.x &&
				pNode->GetPosition().x - pNode->GetBoundingRadius() < regions[i]._max.x &&
				pNode->GetPosition().y + pNode->GetBoundingRadius() > regions[i]._min.y &&
				pNode->GetPosition().y - pNode->GetBoundingRadius() < regions[i]._max.y &&
				pNode->GetPosition().z + pNode->GetBoundingRadius() > regions[i]._min.z &&
				pNode->GetPosition().z - pNode->GetBoundingRadius() < regions[i]._max.z)
			{
				//Add node to this octant's list
				pNodeLists[i].push_back(pNode);
			}
		}

		m_octants[i] = new Octant(regions[i], pNodeLists[i], this);

		//Recursively divide each sub octant where required
		m_octants[i]->divideOctant();
	}
	
	//Remove all the nodes from this octant because they've all been added to child octants
	m_physicsNodes.clear();
}

void Octant::genPairs(std::vector<CollisionPair>& colPairs)
{
	//if this is a leaf so generate pairs
	if (m_physicsNodes.size() > 0)
	{
		PhysicsNode *pnodeA, *pnodeB;

		std::vector<PhysicsNode*> rootNodes = getRoot()->getPhysicsNodes();

		//Calculate octree collision pairs
		for (size_t i = 0; i < m_physicsNodes.size() - 1; ++i)
		{
			for (size_t j = i + 1; j < m_physicsNodes.size(); ++j)
			{
				pnodeA = m_physicsNodes[i];
				pnodeB = m_physicsNodes[j];

				//if both objects are at rest then there is no need to check for collision
				if (pnodeA->GetAtRest() && pnodeB->GetAtRest() &&
					pnodeA->GetTimeSinceRestCheck() < 10.0f && pnodeB->GetTimeSinceRestCheck() < 10.0f)
				{
					continue;
				}

				//pnodeA->SetTimeSinceRestCheck()

				//Check they both atleast have collision shapes
				if (pnodeA->GetCollisionShape() != NULL
					&& pnodeB->GetCollisionShape() != NULL)
				{
					CollisionPair cp;
					cp.pObjectA = pnodeA;
					cp.pObjectB = pnodeB;

					bool pairFound = false;

					//Only add if this pair hasn't been added previously
					for (CollisionPair cp2 : colPairs)
					{
						if (cp == cp2)
						{
							pairFound = true;
							continue;
						}
					}
					if (!pairFound)
					{
						colPairs.push_back(cp);
					}
				}
			}
		}
	}
	//This is not a leaf. Generate pairs for children
	else
	{
		for (size_t i = 0; i < NUM_OCTANTS; ++i)
		{
			if (m_octants[i])
			{
				m_octants[i]->genPairs(colPairs);
			}
		}
	}
}

void Octant::debugDraw()
{
	/*
	*      4-------+
	*	  /|      /|
	*	 / |     / |
	*	Y--|----3  |
	*	|  Z----|--2
	*	| /     | /
	*	1-------X
	*/

	float lineThickness = 0.1f;

	//1
	Vector3 corner = m_region._min;
	NCLDebug::DrawThickLine(corner, Vector3(m_region._max.x,	corner.y,			corner.z		), lineThickness, Vector4(1.0f, 0.0f, 0.0f, 1.0f));
	NCLDebug::DrawThickLine(corner, Vector3(corner.x,			m_region._max.y,	corner.z		), lineThickness, Vector4(0.0f, 1.0f, 0.0f, 1.0f));
	NCLDebug::DrawThickLine(corner, Vector3(corner.x,			corner.y,			m_region._max.z	), lineThickness, Vector4(0.0f, 0.0f, 1.0f, 1.0f));

	//2
	corner = Vector3(m_region._max.x, m_region._min.y, m_region._max.z);
	NCLDebug::DrawThickLine(corner, Vector3(m_region._min.x,	corner.y,			corner.z		), lineThickness, Vector4(1.0f, 0.0f, 0.0f, 1.0f));
	NCLDebug::DrawThickLine(corner, Vector3(corner.x,			m_region._max.y,	corner.z		), lineThickness, Vector4(0.0f, 1.0f, 0.0f, 1.0f));
	NCLDebug::DrawThickLine(corner, Vector3(corner.x,			corner.y,			m_region._min.z	), lineThickness, Vector4(0.0f, 0.0f, 1.0f, 1.0f));

	//3
	corner = Vector3(m_region._max.x, m_region._max.y, m_region._min.z);
	NCLDebug::DrawThickLine(corner, Vector3(m_region._min.x,	corner.y,			corner.z		), lineThickness, Vector4(1.0f, 0.0f, 0.0f, 1.0f));
	NCLDebug::DrawThickLine(corner, Vector3(corner.x,			m_region._min.y,	corner.z		), lineThickness, Vector4(0.0f, 1.0f, 0.0f, 1.0f));
	NCLDebug::DrawThickLine(corner, Vector3(corner.x,			corner.y,			m_region._max.z	), lineThickness, Vector4(0.0f, 0.0f, 1.0f, 1.0f));

	//4
	corner = Vector3(m_region._min.x, m_region._max.y, m_region._max.z);
	NCLDebug::DrawThickLine(corner, Vector3(m_region._max.x,	corner.y,			corner.z		), lineThickness, Vector4(1.0f, 0.0f, 0.0f, 1.0f));
	NCLDebug::DrawThickLine(corner, Vector3(corner.x,			m_region._min.y,	corner.z		), lineThickness, Vector4(0.0f, 1.0f, 0.0f, 1.0f));
	NCLDebug::DrawThickLine(corner, Vector3(corner.x,			corner.y,			m_region._min.z	), lineThickness, Vector4(0.0f, 0.0f, 1.0f, 1.0f));

	for (int i = 0; i < NUM_OCTANTS; ++i)
	{
		if (m_octants[i])
		{
			m_octants[i]->debugDraw();
		}
	}
}

Octant* Octant::getRoot()
{
	if (m_parent == NULL)
	{
		return this;
	}
	else
	{
		return m_parent->getRoot();
	}
}
