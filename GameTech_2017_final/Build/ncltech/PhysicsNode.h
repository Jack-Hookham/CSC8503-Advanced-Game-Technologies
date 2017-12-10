/******************************************************************************
Class: PhysicsNode
Implements:
Author: 
	Author: Pieran Marris      <p.marris@newcastle.ac.uk> and YOU!
Description: 

	A class to store all of the properties that define a physical object in the world.

	This class shouldn't need to be changed for the course of the physics tutorials, though
	will be used extensively. 

	As we go through the physics tutorials, we will be adding more and more of these parameters into
	our physics engine and processing them as required. 

	The first ones (and all that is needed to give an object movement) we will use are:
		<---------LINEAR-------------->
		Vector3		position;						
		Vector3		linVelocity;
		Vector3		force;
		float		invMass;

		<----------ANGULAR-------------->
		Quaternion  orientation;
		Vector3		angVelocity;
		Vector3		torque;
		Matrix3     invInertia;


*//////////////////////////////////////////////////////////////////////////////

#define VELOCITY_FRAMES 10		//Number of frames used to calculate rest state

#pragma once
#include <nclgl\Quaternion.h>
#include <nclgl\Matrix3.h>
#include "CollisionShape.h"
#include <functional>

struct CollisionPair	//Forms the output of the broadphase collision detection
{
	PhysicsNode* pObjectA;
	PhysicsNode* pObjectB;

	bool operator==(CollisionPair rhs)
	{
		return pObjectA == rhs.pObjectA && pObjectB == rhs.pObjectB;
	}

	bool operator!=(CollisionPair rhs)
	{
		return pObjectA != rhs.pObjectA || pObjectB != rhs.pObjectB;
	}
};

class PhysicsNode;

//Callback function called whenever a collision is detected between two objects
//Params:
//	PhysicsNode* this_obj			- The current object class that contains the callback
//	PhysicsNode* colliding_obj	- The object that is colliding with the given object
//Return:
//  True	- The physics engine should process the collision as normal
//	False	- The physics engine should drop the collision pair and not do any further collision resolution/manifold generation
//			  > This can be useful for AI to see if a player/agent is inside an area/collision volume
typedef std::function<bool(PhysicsNode* this_obj, PhysicsNode* colliding_obj)> PhysicsCollisionCallback;


//Callback function called whenever this physicsnode's world transform is updated
//Params:
//	const Matrix4& transform - New World transform of the physics node
typedef std::function<void(const Matrix4& transform)> PhysicsUpdateCallback;


class GameObject;
class PhysicsNode
{
public:
	PhysicsNode()
		: position(0.0f, 0.0f, 0.0f)
		, linVelocity(0.0f, 0.0f, 0.0f)
		, linVelocities{Vector3(10.0f, 10.0f, 10.0f)}
		, force(0.0f, 0.0f, 0.0f)
		, invMass(0.0f)
		, orientation(0.0f, 0.0f, 0.0f, 1.0f)
		, angVelocity(0.0f, 0.0f, 0.0f)
		, angVelocities{ Vector3(10.0f, 10.0f, 10.0f) }
		, torque(0.0f, 0.0f, 0.0f)
		, invInertia(Matrix3::ZeroMatrix)
		, collisionShape(NULL)
		, friction(0.5f)
		, elasticity(0.9f)
		, atRest(false)
	{
	}

	virtual ~PhysicsNode()
	{
		SAFE_DELETE(collisionShape);
	}


	//<-------- Integration --------->
	// Called automatically by PhysicsEngine on all physics nodes each frame
	void IntegrateForVelocity(float dt);
	//<-- Between calling these two functions the physics engine will solve velocity to get 'true' final velocity -->
	void IntegrateForPosition(float dt);

	inline void UpdateRestTimer(float dt) { timeSinceRestCheck += dt; }

	//<--------- GETTERS ------------->
	inline GameObject*			GetParent()					const { return parent; }

	inline float				GetElasticity()				const { return elasticity; }
	inline float				GetFriction()				const { return friction; }

	inline const Vector3&		GetPosition()				const { return position; }
	inline const Vector3&		GetLinearVelocity()			const { return linVelocity; }
	inline const Vector3&		GetForce()					const { return force; }
	inline float				GetInverseMass()			const { return invMass; }

	inline const Quaternion&	GetOrientation()			const { return orientation; }
	inline const Vector3&		GetAngularVelocity()		const { return angVelocity; }
	inline const Vector3&		GetTorque()					const { return torque; }
	inline const Matrix3&		GetInverseInertia()			const { return invInertia; }

	inline CollisionShape*		GetCollisionShape()			const { return collisionShape; }

	inline const Matrix4&		GetWorldSpaceTransform()    const { return worldTransform; }

	inline const float			GetBoundingRadius()			const { return boundingRadius; }
	inline const bool			GetAtRest()					const { return atRest; }
	inline const float			GetTimeSinceRestCheck()		const { return timeSinceRestCheck; }

	inline const int			GetSoftBodyID()				const { return softBodyID; }


	//<--------- SETTERS ------------->
	inline void SetParent(GameObject* obj)							{ parent = obj; }

	inline void SetElasticity(float elasticityCoeff)				{ elasticity = elasticityCoeff; }
	inline void SetFriction(float frictionCoeff)					{ friction = frictionCoeff; }

	inline void SetPosition(const Vector3& v)						{ position = v; FireOnUpdateCallback(); }
	inline void SetLinearVelocity(const Vector3& v)					{ linVelocity = v; }
	inline void SetForce(const Vector3& v)							{ force = v; }
	inline void SetInverseMass(const float& v)						{ invMass = v; }

	inline void SetOrientation(const Quaternion& v)					{ orientation = v; FireOnUpdateCallback(); }
	inline void SetAngularVelocity(const Vector3& v)				{ angVelocity = v; }
	inline void SetTorque(const Vector3& v)							{ torque = v; }
	inline void SetInverseInertia(const Matrix3& v)					{ invInertia = v; }

	inline void SetCollisionShape(CollisionShape* colShape)
	{ 
		if (collisionShape) collisionShape->SetParent(NULL);
		collisionShape = colShape;
		if (collisionShape) collisionShape->SetParent(this);
	}

	inline void SetBoundingRadius(const float radius) { boundingRadius = radius; }
	inline void SetAtRest(const bool rest) { atRest = rest; }
	inline void SetTimeSinceRestCheck(const float time) { timeSinceRestCheck = time; }

	inline void SetSoftBodyID(const int id) { softBodyID = id; }

	//<---------- CALLBACKS ------------>
	inline void SetOnCollisionCallback(PhysicsCollisionCallback callback) { onCollisionCallback = callback; }
	inline bool FireOnCollisionEvent(PhysicsNode* obj_a, PhysicsNode* obj_b)
	{
		return (onCollisionCallback) ? onCollisionCallback(obj_a, obj_b) : true;
	}

	inline void SetOnUpdateCallback(PhysicsUpdateCallback callback) { onUpdateCallback = callback; }
	inline void FireOnUpdateCallback()
	{
		//Build world transform
		worldTransform = orientation.ToMatrix4();
		worldTransform.SetPositionVector(position);
			
		//Fire the OnUpdateCallback, notifying GameObject's and other potential
		// listeners that this PhysicsNode has a new world transform.
		if (onUpdateCallback) onUpdateCallback(worldTransform);
	}
	
	void DrawBoundingRadius();

	float GetMinX() { return minX; }
	float GetMaxX() { return maxX; }
	void SetMinX(float x) { minX = x; }
	void SetMaxX(float x) { maxX = x; }

	float GetMinZ() { return minZ; }
	float GetMaxZ() { return maxZ; }
	void SetMinZ(float z) { minZ = z; }
	void SetMaxZ(float z) { maxZ = z; }

	//Update the velocity arrays for the previous frames
	void UpdateVelocities();
	//Determine the current rest state from the previous frame velocities
	void DetermineRestState();
	//Set the previous frame velocities to > 0 (used after dragging)
	void ResetVelocities();

protected:
	//Useful parameters
	GameObject*				parent;
	Matrix4					worldTransform;
	PhysicsUpdateCallback	onUpdateCallback;
	
	float					boundingRadius;		//Bounding radius used for broadphase collision checks

//Added in Tutorial 2
	//<---------LINEAR-------------->
	Vector3		position;
	Vector3		linVelocity;
	//Track previous frame linVelocities for determining rest state - initialise to value > 0
	Vector3		linVelocities[VELOCITY_FRAMES];		
	Vector3		force;
	float		invMass;

	//<----------ANGULAR-------------->
	Quaternion  orientation;
	Vector3		angVelocity;
	//Track previous frame angVelocities for determining rest state  - initialise to value > 0
	Vector3		angVelocities[VELOCITY_FRAMES];
	Vector3		torque;
	Matrix3     invInertia;


//Added in Tutorial 4/5
	//<----------COLLISION------------>
	CollisionShape*				collisionShape;
	PhysicsCollisionCallback	onCollisionCallback;


//Added in Tutorial 5
	//<--------MATERIAL-------------->
	float				elasticity;		///Value from 0-1 definiing how much the object bounces off other objects
	float				friction;		///Value from 0-1 defining how much the object can slide off other objects

	float minX;
	float maxX;
	float minZ;
	float maxZ;

	bool atRest;

	//Track the time since the object was last checked to be at rest.
	//Sometimes objects will have another object moved from underneath it
	//without a collision happening so the top object will think that
	//it should still be at rest, when really it should start to fall.
	//To fix this I occasionally stop all resting objects from resting
	float timeSinceRestCheck = 0.0f;

	//Give each soft body an id
	//All physics nodes in that body will also be given that id
	//They can then be checked for the same id to prevent collisions
	//between nodes in the same soft body
	//To allow different soft bodies to collide with each other
	//each soft body should have a unique id
	//None soft bodies should just use NULL
	int softBodyID = NULL;
};