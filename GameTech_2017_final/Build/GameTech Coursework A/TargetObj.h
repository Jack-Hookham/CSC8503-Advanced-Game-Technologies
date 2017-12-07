
#pragma once
#include <ncltech\GameObject.h>
#include <ncltech\CuboidCollisionShape.h>
#include <nclgl\NCLDebug.h>

class TargetObj : public GameObject
{
public:
	TargetObj(
		const std::string& name,
		const Vector3& pos,
		const Vector3& halfdims,
		bool physics_enabled = false,
		float inverse_mass = 0.0f,
		bool collidable = true,				
		bool dragable = true,
		const Vector4& color = Vector4(1.0f, 1.0f, 1.0f, 1.0f),
		bool rest = false,
		CommonMeshes::MeshType meshType = CommonMeshes::MeshType::TARGET_CUBE)
		: GameObject(name)
	{
		RenderNode* rnode = new RenderNode();

		RenderNode* dummy = new RenderNode(CommonMeshes::Meshes()[meshType], Vector4(0.1f, 1.0f, 0.1f, 1.0f));
		dummy->SetTransform(Matrix4::Scale(halfdims));
		rnode->AddChild(dummy);

		float radius = halfdims.Length();
		rnode->SetTransform(Matrix4::Translation(pos));
		rnode->SetBoundingRadius(radius);

		PhysicsNode* pnode = NULL;
		if (physics_enabled)
		{
			pnode = new PhysicsNode();
			pnode->SetPosition(pos);
			pnode->SetInverseMass(inverse_mass);
			pnode->SetBoundingRadius(radius);

			if (!collidable)
			{
				//Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
				pnode->SetInverseInertia(CuboidCollisionShape(halfdims).BuildInverseInertia(inverse_mass));
			}
			else
			{
				CollisionShape* pColshape = new CuboidCollisionShape(halfdims);
				pnode->SetCollisionShape(pColshape);
				pnode->SetInverseInertia(pColshape->BuildInverseInertia(inverse_mass));
			}
		}

		SetRender(rnode);
		SetPhysics(pnode);
	}
	
	//Score access
	inline void SetScore(const int value) { score = value; }
	inline void SetScoreUpdating(const bool value) { scoreUpdating = value; }
	inline void SetTargetOn(const bool value) { targetOn = value; }

	inline const int GetScore() const { return score; }
	inline const bool GetScoreUpdating() const { return scoreUpdating; }
	inline const bool GetTargetOn() const { return targetOn; }
	inline const float GetTargetTimer() const { return targetTimer; }
	inline const float GetUpdateTimer() const { return updateTimer; }

	inline void ResetTargetTimer() { targetTimer = 0.0f; }
	inline void ResetUpdateTimer() { updateTimer = 0.0f; }

	inline void UpdateTargetTimer(const float dt) { targetTimer += dt; }
	inline void UpdateUpdateTimer(const float dt) { updateTimer += dt; }

protected:

	//Target stuff
	int score = 0;

	bool scoreUpdating = false;
	bool targetOn = true;
	float targetTimer = 0.0f;
	float updateTimer = 0.0f;
};