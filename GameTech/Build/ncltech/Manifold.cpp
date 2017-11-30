#include "Manifold.h"
#include <nclgl\Matrix3.h>
#include <nclgl\NCLDebug.h>
#include "PhysicsEngine.h"
#include <algorithm>

Manifold::Manifold() 
	: pnodeA(NULL)
	, pnodeB(NULL)
{
}

Manifold::~Manifold()
{

}

void Manifold::Initiate(PhysicsNode* nodeA, PhysicsNode* nodeB)
{
	contactPoints.clear();

	pnodeA = nodeA;
	pnodeB = nodeB;
}

void Manifold::ApplyImpulse()
{
	for (ContactPoint& contact : contactPoints)
	{
		SolveContactPoint(contact);
	}
}


void Manifold::SolveContactPoint(ContactPoint& c)
{	
	/* TUTORIAL 6 CODE */

	if (pnodeA->GetInverseMass() + pnodeB->GetInverseMass() == 0.0f)
	{
		return;
	}

	Vector3 r1 = c.relPosA;
	Vector3 r2 = c.relPosB;

	Vector3 v0 = pnodeA->GetLinearVelocity() + Vector3::Cross(pnodeA->GetAngularVelocity(), r1);
	Vector3 v1 = pnodeB->GetLinearVelocity() + Vector3::Cross(pnodeB->GetAngularVelocity(), r2);

	Vector3 normal = c.colNormal;
	Vector3 dv = v0 - v1;

	//Collision Resolution
	{
		float constraintMass = (pnodeA->GetInverseMass() + pnodeB->GetInverseMass()) +
			Vector3::Dot(normal, Vector3::Cross(pnodeA->GetInverseInertia() *
			Vector3::Cross(r1, normal), r1) + Vector3::Cross(pnodeB->GetInverseInertia() *
			Vector3::Cross(r2, normal), r2));

		//Baumgarte Offset (Adds energy to thesystem to counter 
		//slight solving errors that accumulate over time
		//known as 'constraint drift'

		float b = 0.0f;
		{
			float distance_offset = c.colPenetration;
			float baumgarte_scalar = 0.3f;
			//Amount of force to add to the system to solve error
			float baumgarte_slop = 0.001f;
			//Amount of allowed penetration, ensures a complete manifold each frame
			float penetration_slop = min(c.colPenetration + baumgarte_slop, 0.0f);

			b = -(baumgarte_scalar / PhysicsEngine::Instance()->GetDeltaTime()) * penetration_slop;
		}

		float b_real = max(b, c.elasticity_term + b * 0.2f);
		float jn = -(Vector3::Dot(dv, normal) + b_real) / constraintMass;
		jn = min(jn, 0.0f);

		pnodeA->SetLinearVelocity(pnodeA->GetLinearVelocity() + normal * (jn * pnodeA->GetInverseMass()));
		pnodeB->SetLinearVelocity(pnodeB->GetLinearVelocity() - normal * (jn * pnodeB->GetInverseMass()));

		pnodeA->SetAngularVelocity(pnodeA->GetAngularVelocity() + pnodeA->GetInverseInertia() * Vector3::Cross(r1, normal * jn));
		pnodeB->SetAngularVelocity(pnodeB->GetAngularVelocity() - pnodeB->GetInverseInertia() * Vector3::Cross(r2, normal * jn));
	}

	//Friction
	{
		Vector3 tangent = dv - normal * Vector3::Dot(dv, normal);
		float tangent_len = tangent.Length();

		if (tangent_len > 0.001f)
		{
			tangent = tangent * (1.0f / tangent_len);

			float frictionalMass = (pnodeA->GetInverseMass() + pnodeB->GetInverseMass()) +
				Vector3::Dot(tangent, Vector3::Cross(pnodeA->GetInverseInertia() *
				Vector3::Cross(r1, tangent), r1) + Vector3::Cross(pnodeB->GetInverseInertia() *
				Vector3::Cross(r2, tangent), r2));

			float frictionCoef = sqrtf(pnodeA->GetFriction() * pnodeB->GetFriction());
			float jt = -1 * frictionCoef * Vector3::Dot(dv, tangent) / frictionalMass;

			pnodeA->SetLinearVelocity(pnodeA->GetLinearVelocity() + tangent * (jt * pnodeA->GetInverseMass()));
			pnodeB->SetLinearVelocity(pnodeB->GetLinearVelocity() - tangent * (jt * pnodeB->GetInverseMass()));

			pnodeA->SetAngularVelocity(pnodeA->GetAngularVelocity() + pnodeA->GetInverseInertia() * 
				Vector3::Cross(r1, tangent * jt));
			pnodeB->SetAngularVelocity(pnodeB->GetAngularVelocity() - pnodeB->GetInverseInertia() *
				Vector3::Cross(r2, tangent * jt));
		}
	}
}

void Manifold::PreSolverStep(float dt)
{
	for (ContactPoint& contact : contactPoints)
	{
		UpdateConstraint(contact);
	}
}

void Manifold::UpdateConstraint(ContactPoint& c)
{
	//Reset total impulse forces computed this physics timestep 
	c.sumImpulseContact = 0.0f;
	c.sumImpulseFriction = Vector3(0.0f, 0.0f, 0.0f);
	c.b_term = 0.0f;

	/* TUTORIAL 6 CODE */

	//Compute Elasticity Term - must be computed prior to solving
	//ANY constraints otherwise the objects velocities may have
	//already changed in a different constraint and the elasticity
	//force will no longer be correct.

	{
		const float elasticity = sqrtf(pnodeA->GetElasticity() * pnodeB->GetElasticity());

		float elasticity_term = elasticity * Vector3::Dot(c.colNormal,
			pnodeA->GetLinearVelocity() + Vector3::Cross(c.relPosA, pnodeA->GetAngularVelocity()) -
			pnodeB->GetLinearVelocity() - Vector3::Cross(c.relPosB, pnodeB->GetAngularVelocity()));

		if (elasticity_term < 0.0f)
		{
			c.elasticity_term = 0.0f;
		}
		else
		{
			//Elasticity slop here is used to make objects come to
			//rest quicker. It works out if the elastic term is less
			//than a given value (0.2 m/s here) and if it is, then we
			//assume it is too small to see and should ignore the
			//elasticity calculation . Most noticable when you have a
			//stack of objects, without this they will jitter alot.

			const float elasticity_slop = 0.2f;

			if (elasticity_term < elasticity_slop)
			{
				elasticity_term = 0.0f;
			}

			c.elasticity_term = elasticity_term;
		}
	}
}

void Manifold::AddContact(const Vector3& globalOnA, const Vector3& globalOnB, const Vector3& normal, const float& penetration)
{
	//Get relative offsets from each object centre of mass
	// Used to compute rotational velocity at the point of contact.
	Vector3 r1 = (globalOnA - pnodeA->GetPosition());
	Vector3 r2 = (globalOnB - pnodeB->GetPosition());

	//Create our new contact descriptor
	ContactPoint contact;
	contact.relPosA = r1;
	contact.relPosB = r2;
	contact.colNormal = normal;
	contact.colNormal.Normalise();
	contact.colPenetration = penetration;

	contactPoints.push_back(contact);


	//What a stupid function!
	// - Manifold's normally persist over multiple frames, as in two colliding objects
	//   (especially in the case of stacking) will likely be colliding in a similar 
	//   setup the following couple of frames. So the accuracy therefore can be increased
	//   by using persistent manifolds, which will only be deleted when the two objects
	//   fail a narrowphase check. This means the manifolds can be quite busy, with lots of
	//   new contacts per frame, but to solve any convex manifold in 3D you really only need
	//   3 contacts (4 max), so tldr; perhaps you want persistent manifolds.. perhaps
	//   you want to put some code here to sort contact points.. perhaps this comment is even 
	//   more pointless than a passthrough function.. perhaps I should just stop typ
}

void Manifold::DebugDraw() const
{
	if (contactPoints.size() > 0)
	{
		//Loop around all contact points and draw them all as a line-loop
		Vector3 globalOnA1 = pnodeA->GetPosition() + contactPoints.back().relPosA;
		for (const ContactPoint& contact : contactPoints)
		{
			Vector3 globalOnA2 = pnodeA->GetPosition() + contact.relPosA;
			Vector3 globalOnB = pnodeB->GetPosition() + contact.relPosB;

			//Draw line to form area given by all contact points
			NCLDebug::DrawThickLineNDT(globalOnA1, globalOnA2, 0.02f, Vector4(0.0f, 1.0f, 0.0f, 1.0f));

			//Draw descriptors for indivdual contact point
			NCLDebug::DrawPointNDT(globalOnA2, 0.05f, Vector4(0.0f, 0.5f, 0.0f, 1.0f));
			NCLDebug::DrawThickLineNDT(globalOnB, globalOnA2, 0.01f, Vector4(1.0f, 0.0f, 1.0f, 1.0f));

			globalOnA1 = globalOnA2;
		}
	}
}