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
		const bool collidable, const bool draggable);
	~SoftBody();

	void GenerateBody();
	void GeneratePhysicsNodes();
	void GeneratePhysicsConstraints();

	inline GameObjectExtended* SoftObject() { return softObject; }

	//Create a spring constraint between the current node and the node 
	//in the direction specified
	//Pass in the x and y index of the current node
	void ConnectRight(const int x, const int y);
	void ConnectUp(const int x, const int y);
	void ConnectRightUp(const int x, const int y);
	void ConnectLeftUp(const int x, const int y);

protected:
	std::string m_name;
	int m_numNodesX;
	int m_numNodesY;
	float m_nodeSeparation;
	Vector3 m_position;
	float m_invNodeMass;
	bool m_collidable;
	bool m_draggable;

	std::vector<PhysicsNode*> m_pnodes;
	GameObjectExtended* softObject;
};

