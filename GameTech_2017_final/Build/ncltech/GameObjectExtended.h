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

	void GenerateMesh();

	//inline void RegisterPhysicsToRenderTransformCallback()
	//{
	//	for (int i = 0; i < m_physicsNodes.size(); ++i)
	//	{
	//		if (m_physicsNodes[i] && renderNode)
	//		{
	//			m_physicsNodes[i]->SetOnUpdateCallback(
	//				std::bind(
	//					&RenderNode::SetTransform,		// Function to call
	//					renderNode,					// Constant parameter (in this case, as a member function, we need a 'this' parameter to know which class it is)
	//					std::placeholders::_1)			// Variable parameter(s) that will be set by the callback function
	//			);
	//		}
	//	}
	//}

	//inline void UnregisterPhysicsToRenderTransformCallback()
	//{
	//	for (int i = 0; i < m_physicsNodes.size(); ++i)
	//	{
	//		if (m_physicsNodes[i])
	//		{
	//			m_physicsNodes[i]->SetOnUpdateCallback([](const Matrix4&) {});
	//		}
	//	}
	//}

protected:
	std::vector<PhysicsNode*> m_physicsNodes;
	Mesh* mesh;
};

