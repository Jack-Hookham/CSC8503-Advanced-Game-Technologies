#include "SoftBody.h"

SoftBody::SoftBody(const std::string& name, const int nodesX, const int nodesY, 
	const float separation, const Vector3 pos, const float invNodeMass, 
	const bool collidable, const bool draggable, const int id, GLuint texture)
{
	m_name = name;
	m_numNodesX = nodesX;
	m_numNodesY = nodesY;
	m_nodeSeparation = separation;
	m_position = pos;
	m_invNodeMass = invNodeMass;
	m_collidable = collidable;
	m_draggable = draggable;
	m_nodeRadius = m_nodeSeparation * 0.5f;
	m_id = id;
	m_texture = texture;

	GenerateBody();
}

SoftBody::~SoftBody()
{
	SAFE_DELETE(m_mesh);
}

void SoftBody::GenerateBody()
{
	//Create the physics nodes making up the soft body
	GeneratePhysicsNodes();
	//Create the contraints connecting the nodes together
	GeneratePhysicsConstraints();
	//Create the mesh for the whole 
	m_mesh = GenerateMesh();
	m_mesh->SetTexture(m_texture);

	RenderNode* rnode = new RenderNode();

	RenderNode* dummy = new RenderNode(m_mesh, Vector4(1.0f, 1.0f, 1.0f, 1.0f), false);
	dummy->SetTransform(Matrix4::Scale(Vector3(m_nodeRadius, m_nodeRadius, m_nodeRadius)));
	rnode->AddChild(dummy);

	rnode->SetTransform(Matrix4::Translation(m_position));
	rnode->SetBoundingRadius(m_nodeRadius);
	rnode->SetColorRecursive(Vector4(1.0f, 1.0f, 1.0f, 1.0f));

	softObject = new GameObjectExtended(m_name, rnode, m_pnodes);

	m_pnodes[0]->SetOnUpdateCallback(
		std::bind(&SoftBody::UpdateMeshVertices,
			this,
			std::placeholders::_1));

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
			pnode->SetBoundingRadius(m_nodeRadius);
			pnode->SetSoftBodyID(m_id);

			if (!m_collidable)
			{
				pnode->SetInverseInertia(SphereCollisionShape(m_nodeRadius).BuildInverseInertia(m_invNodeMass));
			}
			else
			{
				CollisionShape* pColshape = new SphereCollisionShape(m_nodeRadius);
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
			//CASE 1: 3 constraints
			if (x == 0 && y < m_numNodesY - 1)
			{
				ConnectRight(x, y);
				ConnectUp(x, y);
				ConnectRightUp(x, y);
			}
			//CASE 2: 4 constraints
			else if (x > 0 && x < m_numNodesX - 1 && y < m_numNodesY - 1)
			{
				ConnectRight(x, y);
				ConnectUp(x, y);
				ConnectRightUp(x, y);
				ConnectLeftUp(x, y);
			}
			//CASE 3: 2 constraints
			else if (x == m_numNodesX - 1 && y < m_numNodesY - 1)
			{
				ConnectUp(x, y);
				ConnectLeftUp(x, y);
			}
			//CASE 4: 1 constraint
			else if (x < m_numNodesX - 1 && y == m_numNodesY - 1)
			{
				ConnectRight(x, y);
			}
			//CASE 5: 0 constraints (top right corner)
		}
	}
}

Mesh* SoftBody::GenerateMesh()
{
	//Generate a quad at each node
	Mesh* m = new Mesh();
	m->numVertices = (m_numNodesX) * (m_numNodesY) * 6;

	m->vertices = new Vector3[m->numVertices];
	m->textureCoords = new Vector2[m->numVertices];
	m->colours = new Vector4[m->numVertices];
	m->normals = new Vector3[m->numVertices];
	m->tangents = new Vector3[m->numVertices];

	float invNodeRadius = 1.0f / m_nodeRadius;

	float texConstX = 1.0f / (m_numNodesX - 1);
	float texConstY = 1.0f / (m_numNodesY - 1);

	for (int x = 0; x < m_numNodesX - 1; ++x)
	{
		for (int y = 0; y < m_numNodesY - 1; ++y)
		{
			PhysicsNode* pnodeCurrent = m_pnodes[x * m_numNodesY + y];
			int vertexIndex = (x * m_numNodesY + y) * 6;

			//Bottom triangle
			m->vertices[vertexIndex] = (pnodeCurrent->GetPosition() - m_position) * invNodeRadius;
			m->vertices[vertexIndex + 1] = (GetRight(x, y)->GetPosition() - m_position) * invNodeRadius;
			m->vertices[vertexIndex + 2] = (GetUp(x, y)->GetPosition() - m_position) * invNodeRadius;

			//Top triangle
			m->vertices[vertexIndex + 3] = (GetRight(x, y)->GetPosition() - m_position) * invNodeRadius;
			m->vertices[vertexIndex + 4] = (GetRightUp(x, y)->GetPosition() - m_position) * invNodeRadius;
			m->vertices[vertexIndex + 5] = (GetUp(x, y)->GetPosition() - m_position) * invNodeRadius;
			
			m->textureCoords[vertexIndex] = Vector2(x * texConstX, 1 - (y * texConstY));
			m->textureCoords[vertexIndex + 1] = Vector2((x + 1) * texConstX, 1 - (y * texConstY));
			m->textureCoords[vertexIndex + 2] = Vector2(x * texConstX, 1 - ((y + 1) * texConstY));

			m->textureCoords[vertexIndex + 3] = Vector2((x + 1) * texConstX, 1 - (y * texConstY));
			m->textureCoords[vertexIndex + 4] = Vector2((x + 1) * texConstX, 1 - ((y + 1) * texConstY));
			m->textureCoords[vertexIndex + 5] = Vector2(x * texConstX, 1 - ((y + 1) * texConstY));

			for (int i = vertexIndex; i < vertexIndex + 6; ++i)
			{
				m->colours[i] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
				m->normals[i] = Vector3(0.0f, 0.0f, -1.0f);
				m->tangents[i] = Vector3(1.0f, 0.0f, 0.0f);
			}
		}
	}

	m->BufferData();
	return m;
}

void SoftBody::UpdateMeshVertices(const Matrix4& mat4)
{
	float invNodeRadius = 1.0f / m_nodeRadius;

	for (int x = 0; x < m_numNodesX - 1; ++x)
	{
		for (int y = 0; y < m_numNodesY - 1; ++y)
		{
			PhysicsNode* pnodeCurrent = m_pnodes[x * m_numNodesY + y];
			int vertexIndex = (x * m_numNodesY + y) * 6;

			//Bottom triangle
			m_mesh->vertices[vertexIndex] = (pnodeCurrent->GetPosition() - m_position) * invNodeRadius;
			m_mesh->vertices[vertexIndex + 1] = (GetRight(x, y)->GetPosition() - m_position) * invNodeRadius;
			m_mesh->vertices[vertexIndex + 2] = (GetUp(x, y)->GetPosition() - m_position) * invNodeRadius;

			//Top triangle
			m_mesh->vertices[vertexIndex + 3] = (GetRight(x, y)->GetPosition() - m_position) * invNodeRadius;
			m_mesh->vertices[vertexIndex + 4] = (GetRightUp(x, y)->GetPosition() - m_position) * invNodeRadius;
			m_mesh->vertices[vertexIndex + 5] = (GetUp(x, y)->GetPosition() - m_position) * invNodeRadius;
		}
	}

	m_mesh->GenerateNormals();
	m_mesh->GenerateTangents();
	m_mesh->DeleteVBO();			//Cleans up the VBO before rebuffering data
	m_mesh->BufferData();
}

void SoftBody::ConnectRight(const int x, const int y)
{
	PhysicsNode* connectFrom = m_pnodes[x * m_numNodesY + y];
	PhysicsNode* connectTo = GetRight(x, y);		//pnode directly to the right

	PhysicsEngine::Instance()->AddConstraint(new SpringConstraint(
		connectFrom,								//Current pnode									
		connectTo,
		connectFrom->GetPosition(),
		connectTo->GetPosition()));
}

void SoftBody::ConnectUp(const int x, const int y)
{
	PhysicsNode* connectFrom = m_pnodes[x * m_numNodesY + y];
	PhysicsNode* connectTo = GetUp(x, y);			//pnode directly up
	
	PhysicsEngine::Instance()->AddConstraint(new SpringConstraint(
		connectFrom,
		connectTo,
		connectFrom->GetPosition(),
		connectTo->GetPosition()));
}

void SoftBody::ConnectRightUp(const int x, const int y)
{
	PhysicsNode* connectFrom = m_pnodes[x * m_numNodesY + y];
	PhysicsNode* connectTo = GetRightUp(x, y);		//pnode right and up
	
	PhysicsEngine::Instance()->AddConstraint(new SpringConstraint(
		connectFrom,
		connectTo,
		connectFrom->GetPosition(),
		connectTo->GetPosition()));
}

void SoftBody::ConnectLeftUp(const int x, const int y)
{
	PhysicsNode* connectFrom = m_pnodes[x * m_numNodesY + y];
	PhysicsNode* connectTo = GetLeftUp(x, y);		//pnode left and up
	
	PhysicsEngine::Instance()->AddConstraint(new SpringConstraint(
		connectFrom,
		connectTo,
		connectFrom->GetPosition(),
		connectTo->GetPosition()));
}

