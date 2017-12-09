#include "SoftBody.h"

SoftBody::SoftBody(const std::string& name, const int nodesX, const int nodesY, 
	const float separation, const Vector3 pos, const float invNodeMass, 
	const bool collidable, const bool draggable)
{
	m_name = name;
	m_numNodesX = nodesX;
	m_numNodesY = nodesY;
	m_nodeSeparation = separation;
	m_position = pos;
	m_invNodeMass = invNodeMass;
	m_collidable = collidable;
	m_draggable = draggable;

	GenerateBody();
}

SoftBody::~SoftBody()
{
}

void SoftBody::GenerateBody()
{
	//Create the physics nodes making up the soft body
	GeneratePhysicsNodes();
	//Create the contraints connecting the nodes together
	GeneratePhysicsConstraints();

	float radius = m_nodeSeparation * 0.5f;
	RenderNode* rnode = new RenderNode();
	
	RenderNode* dummy = new RenderNode(CommonMeshes::Meshes()[CommonMeshes::MeshType::DEFAULT_SPHERE], Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	dummy->SetTransform(Matrix4::Scale(Vector3(radius, radius, radius)));
	rnode->AddChild(dummy);

	rnode->SetTransform(Matrix4::Translation(m_position));
	rnode->SetBoundingRadius(radius);

	softObject = new GameObjectExtended(m_name, rnode, m_pnodes);

	if (m_draggable)
	{
		ScreenPicker::Instance()->RegisterNodeForMouseCallback(
			dummy, //Dummy is the rendernode that actually contains the drawable mesh
			std::bind(&CommonUtils::DragableObjectCallback, softObject, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
		);
	}
}

void SoftBody::GeneratePhysicsNodes()
{
	float radius = m_nodeSeparation * 0.5f;
	for (int x = 0; x < m_numNodesX; ++x)
	{
		for (int y = 0; y < m_numNodesY; ++y)
		{
			Vector3 position = m_position;
			position.x += x * m_nodeSeparation;
			position.y += y * m_nodeSeparation;

			PhysicsNode* pnode = new PhysicsNode();
			pnode->SetPosition(position);
			pnode->SetInverseMass(m_invNodeMass);
			pnode->SetBoundingRadius(radius);

			if (!m_collidable)
			{
				pnode->SetInverseInertia(SphereCollisionShape(radius).BuildInverseInertia(m_invNodeMass));
			}
			else
			{
				CollisionShape* pColshape = new SphereCollisionShape(radius);
				pnode->SetCollisionShape(pColshape);
				pnode->SetInverseInertia(pColshape->BuildInverseInertia(m_invNodeMass));
			}

			m_pnodes.push_back(pnode);
		}
	}
	//Top left and top right corners are stationary
	m_pnodes[m_numNodesY - 1]->SetInverseMass(0.0f);
	m_pnodes[m_pnodes.size() - 1]->SetInverseMass(0.0f);
}

void SoftBody::GeneratePhysicsConstraints()
{
	/*
	 * 5 possible constraint cases
	 * 
	 * y = n   o -----	     o -----	   o
	 *                       
	 *                       
	 *         |   /      \  |  /       \  |
	 *         |  /        \ | /         \ |
	 *         | /	        \|/           \|
	 * y = 1   o -----       o -----       o
	 *         
	 *         
	 *         |   /      \  |  /	    \  |
	 *         |  /	       \ | /	     \ |
	 *         | /	        \|/		      \|
	 * y = 0   o ----        o -----       o
	 *         
	 *         x = 0         x = 1         x = n
	 * 
	 */
	


	for (int x = 0; x < m_numNodesX; ++x)
	{
		for (int y = 0; y < m_numNodesY; ++y)
		{
			PhysicsNode* connectFrom = m_pnodes[x * m_numNodesX + y];
			PhysicsNode* connectTo;
			//CASE 1: 3 constraints
			if (x == 0 && y < m_numNodesY - 1)
			{
				//Right
				connectTo = m_pnodes[(x + 1) * m_numNodesX + y];		//pnode directly to the right
				PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
					connectFrom,											//Current pnode									
					connectTo,								
					connectFrom->GetPosition(),
					connectTo->GetPosition()));

				//Up
				connectTo = m_pnodes[x * m_numNodesX + y + 1];			//pnode directly up
				PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
					connectFrom,								
					connectTo,
					connectFrom->GetPosition(),
					connectTo->GetPosition()));

				//Right, Up
				connectTo = m_pnodes[(x + 1) * m_numNodesX + y + 1];	//pnode right and up
				PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
					connectFrom,
					connectTo,
					connectFrom->GetPosition(),
					connectTo->GetPosition()));
			}
			//CASE 2: 4 constraints
			else if (x > 0 && x < m_numNodesX - 1 && y < m_numNodesY - 1)
			{
				//Right
				connectTo = m_pnodes[(x + 1) * m_numNodesX + y];		//pnode directly to the right
				PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
					connectFrom,											//Current pnode									
					connectTo,
					connectFrom->GetPosition(),
					connectTo->GetPosition()));

				//Up
				connectTo = m_pnodes[x * m_numNodesX + y + 1];			//pnode directly up
				PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
					connectFrom,
					connectTo,
					connectFrom->GetPosition(),
					connectTo->GetPosition()));

				//Right, Up
				connectTo = m_pnodes[(x + 1) * m_numNodesX + y + 1];	//pnode right and up
				PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
					connectFrom,
					connectTo,
					connectFrom->GetPosition(),
					connectTo->GetPosition()));

				//Left, Up
				connectTo = m_pnodes[(x - 1) * m_numNodesX + y + 1];	//pnode left and up
				PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
					connectFrom,
					connectTo,
					connectFrom->GetPosition(),
					connectTo->GetPosition()));
			}
			//CASE 3: 2 constraints
			else if (x == m_numNodesX - 1 && y < m_numNodesY - 1)
			{
				//Up
				connectTo = m_pnodes[x * m_numNodesX + y + 1];			//pnode directly up
				PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
					connectFrom,
					connectTo,
					connectFrom->GetPosition(),
					connectTo->GetPosition()));

				//Left, Up
				connectTo = m_pnodes[(x - 1) * m_numNodesX + y + 1];	//pnode left and up
				PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
					connectFrom,
					connectTo,
					connectFrom->GetPosition(),
					connectTo->GetPosition()));
			}
			//CASE 4: 1 constraint
			else if (x < m_numNodesX - 1 && y == m_numNodesY - 1)
			{
				//Right
				connectTo = m_pnodes[(x + 1) * m_numNodesX + y];		//pnode directly to the right
				PhysicsEngine::Instance()->AddConstraint(new DistanceConstraint(
					connectFrom,											//Current pnode									
					connectTo,
					connectFrom->GetPosition(),
					connectTo->GetPosition()));
			}
			//CASE 5: 0 constraints (top right corner)
		}
	}
}

