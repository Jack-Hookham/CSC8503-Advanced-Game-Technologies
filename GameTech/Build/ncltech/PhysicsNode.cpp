#include "PhysicsNode.h"
#include "PhysicsEngine.h"


void PhysicsNode::IntegrateForVelocity(float dt)
{
	//Apply Gravity
	//Technically gravity here is calculated by formula:
	//(gravity / invMass * invMass * dt)
	//So even though the divide and multiply cancel out, we still
	//need to handle the possibility of divide by zero

	if (invMass > 0.0f)
	{
		linVelocity += PhysicsEngine::Instance()->GetGravity() * dt;
	}

	//Semi-Implicit Euler Integration
	// - See "Update Position" below
	linVelocity += force * invMass * dt;

	//Apply Velocity Damping
	// - This removes a tiny bit of energy from the simulation each update 
	//to stop slight calculation errors accumulating and adding force
	//from nowhere.
	// - In its present from this can be seen as a rough approximation
	//of air resistance, albeit (wrongly?) making the assumption that
	//all objects have the same surface area.
	linVelocity = linVelocity * PhysicsEngine::Instance()->GetDampingFactor();

	//Angular Rotation
	// - These are the exact same calculations as the three lines above,
	//except for rotations rather than positions/
	//	- Mass		-> Torque
	//	- Velocity	-> Rotational Velocity
	//	- Position	-> Orientation
	angVelocity += invInertia * torque * dt;

	//Apply Velocity Damping
	angVelocity = angVelocity * PhysicsEngine::Instance()->GetDampingFactor();
}

/* Between these two functions the physics engine will solve for velocity
   based on collisions/constraints etc. So we need to integrate velocity, solve 
   constraints, then use final velocity to update position. 
*/

void PhysicsNode::IntegrateForPosition(float dt)
{
	//Update Position
	// - Euler integration; works on the assumption that linearvelocity
	//does not change over time (or changes so slightly it doesn't make
	//a difference).
	// - In this scenario , gravity /will/ be increasing velocity over
	//time . The in - accuracy of not taking into account of these changes
	//over time can be visibly seen in tutorial 1.. and thus how better
	//integration schemes lead to better approximations by taking into
	//account of curvature .
	position += linVelocity * dt;

	//Update Orientation
	// - This is a slightly different calculation due to the wierdness
	//of quaternions. It does the same thing as position update
	//(with a slight error) but from what I've seen, is generally the best
	//way to update orientation
	orientation = orientation + Quaternion(angVelocity * dt * 0.5f, 0.0f) * orientation;

	//invIntertia = invIntertia * (Quaternion(angVelocity * dt * 0.5f, 0.0f)
	// * orientation).ToMatrix3();
	//As the above formulation has slight approximation error, we need
	//to normalize our orientation here to stop them accumulation
	//over time
	orientation.Normalise();

	//Finally: Notify any listener's that this PhysicsNode has a new world transform.
	// - This is used by GameObject to set the worldTransform of any RenderNode's. 
	//   Please don't delete this!!!!!
	FireOnUpdateCallback();
}

void PhysicsNode::DrawBoundingRadius()
{
	//Vector3 pos = position;

	//Draw Filled Circle
	NCLDebug::DrawPointNDT(position, boundingRadius, Vector4(1.0f, 1.0f, 1.0f, 0.2f));

	//Draw Perimeter Axes
	Vector3 lastX = position + Vector3(0.0f, 1.0f, 0.0f) * boundingRadius;
	Vector3 lastY = position + Vector3(1.0f, 0.0f, 0.0f) * boundingRadius;
	Vector3 lastZ = position + Vector3(1.0f, 0.0f, 0.0f) * boundingRadius;
	const int nSubdivisions = 20;
	for (int itr = 1; itr <= nSubdivisions; ++itr)
	{
		float angle = itr / float(nSubdivisions) * PI * 2.f;
		float alpha = cosf(angle) * boundingRadius;
		float beta = sinf(angle) * boundingRadius;

		Vector3 newX = position + Vector3(0.0f, alpha, beta);
		Vector3 newY = position + Vector3(alpha, 0.0f, beta);
		Vector3 newZ = position + Vector3(alpha, beta, 0.0f);

		NCLDebug::DrawThickLineNDT(lastX, newX, 0.02f, Vector4(1.0f, 0.3f, 1.0f, 1.0f));
		NCLDebug::DrawThickLineNDT(lastY, newY, 0.02f, Vector4(1.0f, 0.3f, 1.0f, 1.0f));
		NCLDebug::DrawThickLineNDT(lastZ, newZ, 0.02f, Vector4(1.0f, 0.3f, 1.0f, 1.0f));

		lastX = newX;
		lastY = newY;
		lastZ = newZ;
	}
}

void PhysicsNode::UpdateVelocities()
{
	for (int i = 1; i < VELOCITY_FRAMES; ++i)
	{
		linVelocities[i] = linVelocities[i - 1];
		angVelocities[i] = angVelocities[i - 1];
	}
	linVelocities[0] = linVelocity;
	angVelocities[0] = angVelocity;
}

void PhysicsNode::DetermineRestState()
{
	//Whether the node moved in any of the previous frames
	bool movement = false;

	for (int i = 0; i < VELOCITY_FRAMES; ++i)
	{
		//If any of the previous <VELOCITY_FRAMES> frames velocities are greater than <small value> then the node is not at rest 
		if (linVelocities[i].Length() > 0.05f || angVelocities[i].Length() > 0.01f)
		{
			movement = true;
		}
	}
	atRest = movement ? false : true;

}

void PhysicsNode::ResetVelocities()
{
	for (int i = 0; i < VELOCITY_FRAMES; ++i)
	{
		linVelocities[i] = Vector3(10.0f, 10.0f, 10.0f);
		angVelocities[i] = Vector3(10.0f, 10.0f, 10.0f);
	}
}
