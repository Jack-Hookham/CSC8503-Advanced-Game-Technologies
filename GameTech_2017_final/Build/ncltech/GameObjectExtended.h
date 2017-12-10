#pragma once
#include "GameObject.h"

//Similar to GameObject but contains multiple physics nodes

class GameObjectExtended : public GameObject
{
public:
	GameObjectExtended(const std::string& name, RenderNode* renderNode, std::vector<PhysicsNode*>& physicsNodes);
	~GameObjectExtended();

	inline std::vector<PhysicsNode*> GetPhysicsNodes() { return m_physicsNodes; }

	void SetPhysicsNodes(std::vector<PhysicsNode*>& physicsNodes);

protected:
	std::vector<PhysicsNode*> m_physicsNodes;
	Mesh* mesh;
};

