#include "GameObjectExtended.h"

GameObjectExtended::GameObjectExtended(const std::string& name, RenderNode* renderNode, std::vector<PhysicsNode*>& physicsNodes)
{
	friendlyName = name;
	SetRender(renderNode);
	SetPhysicsNodes(physicsNodes);

	RegisterPhysicsToRenderTransformCallback();
}

GameObjectExtended::~GameObjectExtended()
{
	for (int i = 0; i < m_physicsNodes.size(); ++i)
	{
		if (m_physicsNodes[i]) PhysicsEngine::Instance()->RemovePhysicsObject(physicsNode);
		SAFE_DELETE(m_physicsNodes[i]);
	}

	if (renderNode)  GraphicsPipeline::Instance()->RemoveRenderNode(renderNode);
	SAFE_DELETE(renderNode);
}

void GameObjectExtended::SetPhysicsNodes(std::vector<PhysicsNode*>& physicsNodes)
{
	if (physicsNode)
	{
		UnregisterPhysicsToRenderTransformCallback(); //Unregister old callback listener
		physicsNode->SetParent(NULL);
	}

	if (physicsNodes.size() > 0)
	{
		physicsNode = physicsNodes[0];
		RegisterPhysicsToRenderTransformCallback();   //Register new callback listener

		for (int i = 0; i < physicsNodes.size(); ++i)
		{
			physicsNodes[i]->SetParent(this);
		}
	}
}
