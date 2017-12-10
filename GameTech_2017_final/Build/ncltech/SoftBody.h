#pragma once

#include "GameObjectExtended.h"
#include "CommonUtils.h"
#include "SphereCollisionShape.h"
#include "DistanceConstraint.h"
#include "SpringConstraint.h"
#include "ScreenPicker.h"

class SoftBody
{
public:
	SoftBody(const std::string& name, const int nodesX, const int nodesY, 
		const float separation, const Vector3 pos, const float invNodeMass,
		const bool collidable, const bool draggable, const int id = NULL,
		GLuint texture = 0);
	~SoftBody();

	void GenerateBody();
	void GeneratePhysicsNodes();
	void GeneratePhysicsConstraints();
	Mesh* GenerateMesh();


	inline GameObjectExtended* SoftObject() { return softObject; }

protected:
	std::string m_name;
	int m_numNodesX;
	int m_numNodesY;
	float m_nodeSeparation;
	Vector3 m_position;
	float m_invNodeMass;
	bool m_collidable;
	bool m_draggable; 
	float m_nodeRadius;
	int m_id;

	std::vector<PhysicsNode*> m_pnodes;
	Mesh* m_mesh;
	GLuint m_texture;
	GameObjectExtended* softObject;

	void UpdateMeshVertices(const Matrix4& mat4);
	
	//Create a spring constraint between the current node and the node 
	//in the direction specified
	//Pass in the x and y index of the current node
	void ConnectRight(const int x, const int y);
	void ConnectUp(const int x, const int y);
	void ConnectRightUp(const int x, const int y);
	void ConnectLeftUp(const int x, const int y);

	//Get physics node relative to the node indexes passed in
	inline PhysicsNode* GetRight(const int x, const int y)		{ return m_pnodes[(x + 1) * m_numNodesY + y]; }
	inline PhysicsNode* GetUp(const int x, const int y)			{ return m_pnodes[ x      * m_numNodesY + y + 1]; }
	inline PhysicsNode* GetRightUp(const int x, const int y)	{ return m_pnodes[(x + 1) * m_numNodesY + y + 1]; }
	inline PhysicsNode* GetLeftUp(const int x, const int y)		{ return m_pnodes[(x - 1) * m_numNodesY + y + 1]; }
};

