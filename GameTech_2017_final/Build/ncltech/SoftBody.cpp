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
	SAFE_DELETE(m_mesh);

	if (m_texture)
	{
		glDeleteTextures(1, &m_texture);
		m_texture = 0;
	}
}

void SoftBody::GenerateBody()
{
	//Create the physics nodes making up the soft body
	GeneratePhysicsNodes();
	//Create the contraints connecting the nodes together
	GeneratePhysicsConstraints();
	//Create the mesh for the whole 
	m_mesh = GenerateMesh();

	//Initialise textures
	m_texture = SOIL_load_OGL_texture(TEXTUREDIR"MetalSheet.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_COMPRESS_TO_DXT);
	if (m_texture)
	{
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	m_mesh->SetTexture(m_texture);

	float radius = m_nodeSeparation * 0.5f;
	RenderNode* rnode = new RenderNode();

	RenderNode* dummy = new RenderNode(m_mesh, Vector4(1.0f, 1.0f, 1.0f, 1.0f), false);
	dummy->SetTransform(Matrix4::Scale(Vector3(radius, radius, radius)));
	rnode->AddChild(dummy);

	rnode->SetTransform(Matrix4::Translation(m_position));
	rnode->SetBoundingRadius(radius);
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
	for (int x = 0; x < m_numNodesX - 1; ++x)
	{
		for (int y = 0; y < m_numNodesY - 1; ++y)
		{
			int startIndex = (x * m_numNodesX + y) * 6;
			Vector3 nodeOffset = Vector3(x * m_nodeSeparation, y * m_nodeSeparation, 0.0f);

			//Bottom triangle
			m->vertices[startIndex] = Vector3(nodeOffset.x, nodeOffset.y, 0.0f);
			m->vertices[startIndex + 1] = Vector3(nodeOffset.x, nodeOffset.y + m_nodeSeparation, 0.0f);
			m->vertices[startIndex + 2] = Vector3(nodeOffset.x + m_nodeSeparation, nodeOffset.y, 0.0f);

			//Top triangle
			m->vertices[startIndex + 3] = Vector3(nodeOffset.x, nodeOffset.y + m_nodeSeparation, 0.0f);
			m->vertices[startIndex + 4] = Vector3(nodeOffset.x + m_nodeSeparation, nodeOffset.y + m_nodeSeparation, 0.0f);
			m->vertices[startIndex + 5] = Vector3(nodeOffset.x + m_nodeSeparation, nodeOffset.y, 0.0f);

			m->textureCoords[startIndex] = Vector2(0.0f, 0.0f);
			m->textureCoords[startIndex + 1] = Vector2(0.0f, 1.0f);
			m->textureCoords[startIndex + 2] = Vector2(1.0f, 0.0f);
			m->textureCoords[startIndex + 3] = Vector2(0.0f, 1.0f);
			m->textureCoords[startIndex + 4] = Vector2(1.0f, 1.0f);
			m->textureCoords[startIndex + 5] = Vector2(1.0f, 0.0f);

			for (int i = startIndex; i < startIndex + 6; ++i)
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
	float halfDims = m_nodeSeparation * 0.5f;

	for (int x = 0; x < m_numNodesX - 1; ++x)
	{
		for (int y = 0; y < m_numNodesY - 1; ++y)
		{
			PhysicsNode* pnodeCurrent = m_pnodes[x * m_numNodesX + y];
			int vertexIndex = (x * m_numNodesX + y) * 6;

			//Bottom triangle
			m_mesh->vertices[vertexIndex] = pnodeCurrent->GetPosition() - m_position;
			m_mesh->vertices[vertexIndex + 1] = GetRight(x, y)->GetPosition() - m_position;
			m_mesh->vertices[vertexIndex + 2] = GetUp(x, y)->GetPosition() - m_position;

			//Top triangle
			m_mesh->vertices[vertexIndex + 3] = GetRight(x, y)->GetPosition() - m_position;
			m_mesh->vertices[vertexIndex + 4] = GetRightUp(x, y)->GetPosition() - m_position;
			m_mesh->vertices[vertexIndex + 5] = GetUp(x, y)->GetPosition() - m_position;
		}
	}

	m_mesh->GenerateNormals();
	m_mesh->GenerateTangents();
	m_mesh->DeleteVBO();			//Cleans up the VBO before rebuffering data
	m_mesh->BufferData();
}

void SoftBody::ConnectRight(const int x, const int y)
{
	PhysicsNode* connectFrom = m_pnodes[x * m_numNodesX + y];
	PhysicsNode* connectTo = GetRight(x, y);		//pnode directly to the right

	PhysicsEngine::Instance()->AddConstraint(new SpringConstraint(
		connectFrom,													//Current pnode									
		connectTo,
		connectFrom->GetPosition(),
		connectTo->GetPosition()));
}

void SoftBody::ConnectUp(const int x, const int y)
{
	PhysicsNode* connectFrom = m_pnodes[x * m_numNodesX + y];
	PhysicsNode* connectTo = GetUp(x, y);			//pnode directly up
	
	PhysicsEngine::Instance()->AddConstraint(new SpringConstraint(
		connectFrom,
		connectTo,
		connectFrom->GetPosition(),
		connectTo->GetPosition()));
}

void SoftBody::ConnectRightUp(const int x, const int y)
{
	PhysicsNode* connectFrom = m_pnodes[x * m_numNodesX + y];
	PhysicsNode* connectTo = GetRightUp(x, y);	//pnode right and up
	
	PhysicsEngine::Instance()->AddConstraint(new SpringConstraint(
		connectFrom,
		connectTo,
		connectFrom->GetPosition(),
		connectTo->GetPosition()));
}

void SoftBody::ConnectLeftUp(const int x, const int y)
{
	PhysicsNode* connectFrom = m_pnodes[x * m_numNodesX + y];
	PhysicsNode* connectTo = GetLeftUp(x, y);	//pnode left and up
	
	PhysicsEngine::Instance()->AddConstraint(new SpringConstraint(
		connectFrom,
		connectTo,
		connectFrom->GetPosition(),
		connectTo->GetPosition()));
}

