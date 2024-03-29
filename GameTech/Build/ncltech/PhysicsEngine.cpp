#include "PhysicsEngine.h"
#include "GameObject.h"
#include "CollisionDetectionSAT.h"
#include <nclgl\NCLDebug.h>
#include <nclgl\Window.h>
#include <omp.h>
#include <algorithm>



void PhysicsEngine::SetDefaults()
{
	//Variables set here /will/ be reset with each scene
	updateTimestep = 1.0f / 60.f;
	updateRealTimeAccum = 0.0f;
	gravity = Vector3(0.0f, -9.81f, 0.0f);
	dampingFactor = 0.999f;
}

void PhysicsEngine::ToggleOctrees()
{
	useOctrees = !useOctrees;
	if (!useOctrees)
	{
		delete m_octree;
		m_octree = NULL;
	}
	else
	{
		CreateOctree();
	}
}

PhysicsEngine::PhysicsEngine()
{
	//Variables set here will /not/ be reset with each scene
	isPaused = false;  
	debugDrawFlags = DEBUGDRAW_FLAGS_MANIFOLD | DEBUGDRAW_FLAGS_CONSTRAINT | DEBUGDRAW_FLAGS_OCTREE;

	CreateOctree();
	SetDefaults();
}

PhysicsEngine::~PhysicsEngine()
{
	RemoveAllPhysicsObjects();
	SAFE_DELETE(m_octree);
}

void PhysicsEngine::AddPhysicsObject(PhysicsNode* obj)
{
	physicsNodes.push_back(obj);
}

void PhysicsEngine::RemovePhysicsObject(PhysicsNode* obj)
{
	//Lookup the object in question
	auto found_loc = std::find(physicsNodes.begin(), physicsNodes.end(), obj);

	//If found, remove it from the list
	if (found_loc != physicsNodes.end())
	{
		physicsNodes.erase(found_loc);
	}
}

void PhysicsEngine::RemoveAllPhysicsObjects()
{
	//Delete and remove all constraints/collision manifolds
	for (Constraint* c : constraints)
	{
		delete c;
	}
	constraints.clear();

	for (Manifold* m : manifolds)
	{
		delete m;
	}
	manifolds.clear();


	//Delete and remove all physics objects
	// - we also need to inform the (possibly) associated game-object
	//   that the physics object no longer exists
	for (PhysicsNode* obj : physicsNodes)
	{
		if (obj->GetParent()) obj->GetParent()->SetPhysics(NULL);
		delete obj;
	}
	physicsNodes.clear();
}


void PhysicsEngine::Update(float deltaTime)
{
	//The physics engine should run independantly to the renderer
	// - As our codebase is currently single threaded we just need
	//   a way of calling "UpdatePhysics()" at regular intervals
	//   or multiple times a frame if the physics timestep is higher
	//   than the renderers.

	//const int max_updates_per_frame = 5;
	const int max_updates_per_frame = 1;

	if (!isPaused)
	{
		updateRealTimeAccum += deltaTime;
		for (int i = 0; (updateRealTimeAccum >= updateTimestep) && i < max_updates_per_frame; ++i)
		{
			updateRealTimeAccum -= updateTimestep;

			//Additional IsPaused check here incase physics was paused inside one of it's components for debugging or otherwise
			if (!isPaused) UpdatePhysics(); 
		}

		if (updateRealTimeAccum >= updateTimestep)
		{
			NCLDebug::Log("Physics too slow to run in real time!");
			//Drop Time in the hope that it can continue to run faster the next frame
			updateRealTimeAccum = 0.0f;
		}
	}

	for (PhysicsNode* pnode : physicsNodes)
	{
		pnode->UpdateRestTimer(deltaTime);
	}
}


void PhysicsEngine::UpdatePhysics()
{
	for (Manifold* m : manifolds)
	{
		delete m;
	}
	manifolds.clear();

	perfUpdate.UpdateRealElapsedTime(updateTimestep);
	perfBroadphase.UpdateRealElapsedTime(updateTimestep);
	perfNarrowphase.UpdateRealElapsedTime(updateTimestep);
	perfSolver.UpdateRealElapsedTime(updateTimestep);




	//A whole physics engine in 6 simple steps =D
	
	//-- Using positions from last frame --
//1. Broadphase Collision Detection (Fast and dirty)
	perfBroadphase.BeginTimingSection();
	BroadPhaseCollisions();
	perfBroadphase.EndTimingSection();

//2. Narrowphase Collision Detection (Accurate but slow)
	perfNarrowphase.BeginTimingSection();
	NarrowPhaseCollisions();
	perfNarrowphase.EndTimingSection();

	std::random_shuffle(manifolds.begin(), manifolds.end());
	std::random_shuffle(constraints.begin(), constraints.end());

//3. Initialize Constraint Params (precompute elasticity/baumgarte factor etc)
	//Optional step to allow constraints to 
	// precompute values based off current velocities 
	// before they are updated loop below.
	for (Manifold* m : manifolds)
	{
		m->PreSolverStep(updateTimestep);
	}

	for (Constraint* c : constraints)
	{
		c->PreSolverStep(updateTimestep);
	}


//4. Update Velocities
	perfUpdate.BeginTimingSection();
	for (PhysicsNode* obj : physicsNodes)
	{
		if (!obj->GetAtRest())
		{
			obj->IntegrateForVelocity(updateTimestep);
		}
	}
	perfUpdate.EndTimingSection();

//5. Constraint Solver
	perfSolver.BeginTimingSection();

	//------Tut 7-------
	for (size_t i = 0; i < SOLVER_ITERATIONS; ++i)
	{
		for (Manifold* m : manifolds)
		{
			m->ApplyImpulse();
		}

		for (Constraint* c : constraints)
		{
			c->ApplyImpulse();
		}
	}
	perfSolver.EndTimingSection();

//6. Update Positions (with final 'real' velocities)
	perfUpdate.BeginTimingSection();
	for (PhysicsNode* obj : physicsNodes)
	{
		if (!obj->GetAtRest())
		{
			obj->IntegrateForPosition(updateTimestep);
		}
	}
	perfUpdate.EndTimingSection();
}

bool PhysicsEngine::SweepSortFunc(PhysicsNode* nodeA, PhysicsNode* nodeB)
{
	if (nodeA->GetCollisionShape() != NULL && nodeB->GetCollisionShape() != NULL) 
	{
		Vector3 xMinA = Vector3();
		Vector3 xMaxA = Vector3();
		Vector3 xMinB = Vector3();
		Vector3 xMaxB = Vector3();

		//Check X
		nodeA->GetCollisionShape()->GetMinMaxVertexOnAxis(Vector3(1, 0, 0), xMinA, xMaxA);
		nodeA->SetMinX(xMinA.x);
		nodeA->SetMaxX(xMaxA.x);
		nodeB->GetCollisionShape()->GetMinMaxVertexOnAxis(Vector3(1, 0, 0), xMinB, xMaxB);
		nodeB->SetMinX(xMinB.x);
		nodeB->SetMaxX(xMaxB.x);

		//Check Z
		if (xMinA.x < xMinB.x)
		{
			Vector3 zMinA = Vector3();
			Vector3 zMaxA = Vector3();
			Vector3 zMinB = Vector3();
			Vector3 zMaxB = Vector3();

			nodeA->GetCollisionShape()->GetMinMaxVertexOnAxis(Vector3(0, 0, 1), zMinA, zMaxA);
			nodeA->SetMinZ(zMinA.z);
			nodeA->SetMaxZ(zMaxA.z);
			nodeB->GetCollisionShape()->GetMinMaxVertexOnAxis(Vector3(0, 0, 1), zMinB, zMaxB);
			nodeB->SetMinZ(zMinB.z);
			nodeB->SetMaxZ(zMaxB.z);

			return zMinA.z < zMinB.z;
		}
	}
	return true;
}

void PhysicsEngine::BroadPhaseCollisions()
{
	//OCTREES
	//SORT AND SWEEP

	numSphereSphereChecks = 0;
	broadphaseColPairs.clear();

	//Update rest states
	for (PhysicsNode* pnode : physicsNodes)
	{
		pnode->UpdateVelocities();
		pnode->DetermineRestState();
	}

	PhysicsNode *pnodeA, *pnodeB;
	//	The broadphase needs to build a list of all potentially colliding objects in the world,
	//	which then get accurately assesed in narrowphase. If this is too coarse then the system slows down with
	//	the complexity of narrowphase collision checking, if this is too fine then collisions may be missed.

	if (physicsNodes.size() > 0)
	{
		if (useOctrees)
		{
			//Build the octree then generate collision pairs from it
			m_octree->updateObjects(physicsNodes);
			m_octree->buildOctree();
			m_octree->getRoot()->genPairs(broadphaseColPairs);
		}
		else
		{
			//	Brute force approach.
			//  - For every object A, assume it could collide with every other object.. 
			//    even if they are on the opposite sides of the world.
			//Calculate octree collision pairs
			for (size_t i = 0; i < physicsNodes.size() - 1; ++i)
			{
				for (size_t j = i + 1; j < physicsNodes.size(); ++j)
				{
					pnodeA = physicsNodes[i];
					pnodeB = physicsNodes[j];

					//Check they both atleast have collision shapes
					if (pnodeA->GetCollisionShape() != NULL
						&& pnodeB->GetCollisionShape() != NULL)
					{
						CollisionPair cp;
						cp.pObjectA = pnodeA;
						cp.pObjectB = pnodeB;
						broadphaseColPairs.push_back(cp);
					}
				}
			}
		}

		if (useSphereSphere)
		{
			//Cull collision pairs using sphere sphere bounding radius collision check
			SphereSphereCheck();
		}
	}
}

void PhysicsEngine::SphereSphereCheck()
{
	//broadphaseColPairs = SphereSpherePairs();
	PhysicsNode *pnodeA, *pnodeB;

	if (broadphaseColPairs.size() > 0)
	{
		std::vector<CollisionPair> oldPairs = broadphaseColPairs;
		broadphaseColPairs.clear();

		for (CollisionPair cp : oldPairs)
		{
			pnodeA = cp.pObjectA;
			pnodeB = cp.pObjectB;

			//if both objects are at rest then there is no need to check for collision
			if (pnodeA->GetAtRest() && pnodeB->GetAtRest())
			{
				continue;
			}

			//Check they both atleast have collision shapes
			if (pnodeA->GetCollisionShape() != NULL
				&& pnodeB->GetCollisionShape() != NULL)
			{
				numSphereSphereChecks++;

				Vector3 dist = pnodeA->GetPosition() - pnodeB->GetPosition();
				//if distance between the two objects is less than the sum of their bounding radii then they might be colliding
				if (dist.Length() <= pnodeA->GetBoundingRadius() + pnodeB->GetBoundingRadius())
				{
					CollisionPair cp;
					cp.pObjectA = pnodeA;
					cp.pObjectB = pnodeB;
					broadphaseColPairs.push_back(cp);
				}
			}
		}
	}
}


void PhysicsEngine::NarrowPhaseCollisions()
{
	if (broadphaseColPairs.size() > 0)
	{
		//Collision data to pass between detection and manifold generation stages.
		CollisionData colData;				

		//Collision Detection Algorithm to use
		CollisionDetectionSAT colDetect;	

		// Iterate over all possible collision pairs and perform accurate collision detection
		for (size_t i = 0; i < broadphaseColPairs.size(); ++i)
		{
			CollisionPair& cp = broadphaseColPairs[i];

			CollisionShape *shapeA = cp.pObjectA->GetCollisionShape();
			CollisionShape *shapeB = cp.pObjectB->GetCollisionShape();

			colDetect.BeginNewPair(
				cp.pObjectA,
				cp.pObjectB,
				cp.pObjectA->GetCollisionShape(),
				cp.pObjectB->GetCollisionShape());

			//--TUTORIAL 4 CODE--
			// Detects if the objects are colliding
			if (colDetect.AreColliding(&colData))
			{
				//Note: As at the end of tutorial 4 we have very little to do, this is a bit messier
				//      than it should be. We now fire oncollision events for the two objects so they
				//      can handle AI and also optionally draw the collision normals to see roughly
				//      where and how the objects are colliding.

				//Draw collision data to the window if requested
				// - Have to do this here as colData is only temporary. 
				if (debugDrawFlags & DEBUGDRAW_FLAGS_COLLISIONNORMALS)
				{
					NCLDebug::DrawPointNDT(colData._pointOnPlane, 0.1f, Vector4(0.5f, 0.5f, 1.0f, 1.0f));
					NCLDebug::DrawThickLineNDT(colData._pointOnPlane, colData._pointOnPlane - colData._normal * colData._penetration, 0.05f, Vector4(0.0f, 0.0f, 1.0f, 1.0f));
				}

				//Check to see if any of the objects have a OnCollision callback that dont want the objects to physically collide
				bool okA = cp.pObjectA->FireOnCollisionEvent(cp.pObjectA, cp.pObjectB);
				bool okB = cp.pObjectB->FireOnCollisionEvent(cp.pObjectB, cp.pObjectA);

				if (okA && okB)
				{
					/* TUTORIAL 5 CODE */
					//Build full collision manifold that will also handle the
					//collision response between the two objects in the solver
					//stage

					Manifold* manifold = new Manifold();
					manifold->Initiate(cp.pObjectA, cp.pObjectB);

					//Construct contact points that form the perimeter of the collision manifold
					colDetect.GenContactPoints(manifold);

					if (manifold->contactPoints.size() > 0)
					{
						//Add to list of manifolds that need solving
						manifolds.push_back(manifold);
					}
					else
					{
						delete manifold;
					}
				}
			}
		}
	}
}


void PhysicsEngine::DebugRender()
{
	// Draw all collision manifolds
	if (debugDrawFlags & DEBUGDRAW_FLAGS_MANIFOLD)
	{
		for (Manifold* m : manifolds)
		{
			m->DebugDraw();
		}
	}

	// Draw all constraints
	if (debugDrawFlags & DEBUGDRAW_FLAGS_CONSTRAINT)
	{
		for (Constraint* c : constraints)
		{
			c->DebugDraw();
		}
	}

	// Draw all associated collision shapes
	if (debugDrawFlags & DEBUGDRAW_FLAGS_COLLISIONVOLUMES)
	{
		for (PhysicsNode* obj : physicsNodes)
		{
			if (obj->GetCollisionShape() != NULL)
			{
				obj->GetCollisionShape()->DebugDraw();
			}
		}
	}

	if (debugDrawFlags & DEBUGDRAW_FLAGS_OCTREE)
	{
		if (m_octree)
		{
			m_octree->debugDraw();
		}
	}

	if (debugDrawFlags & DEBUGDRAW_FLAGS_BOUNDINGRADIUS)
	{
		for (PhysicsNode* pnode : physicsNodes)
		{
			pnode->DrawBoundingRadius();
		}
	}
}
