/******************************************************************************
Class: CommonMeshes
Implements:
Author: 
	Pieran Marris <p.marris@newcastle.ac.uk>
Description:
	A quick and dirty library of common meshes, saves loading the same common meshes (cube, sphere etc) over and over again.

	These are loaded when the GraphicsPipeline is first initialised and released when it is deleted, so can
	be globally accessed for the entirity of any Scene/GameObject.

*//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nclgl\Mesh.h>
#include <GL\glew.h>

class Scene;

class CommonMeshes
{
	friend class SceneManager; //Initializes/Destroys the given meshes within it's own lifecycle

public:
	enum MeshType
	{
		//Cubes
		DEFAULT_CUBE,
		TARGET_CUBE,
		PORTAL_CUBE,

		//Spheres
		DEFAULT_SPHERE,

		NUM_MESHES
	};
	//To use these resources, just make a copy of the rendernode structure as required
	// e.g RenderNode* cube_copy = new RenderNode(*CommonMeshes::Cube());

	//Mesh Array
	static Mesh** Meshes()		{ return m_pMeshes; }

	//PhysicsEngine Checkerboard - Hidden here for reasons of laziness
	static const GLuint CheckerboardTex()   { return m_pCheckerboardTex; }


protected:
	//Called by SceneRenderer
	static void InitializeMeshes();
	static void ReleaseMeshes();

	static Mesh* m_pMeshes[NUM_MESHES];

	static GLuint m_pCheckerboardTex;
	static GLuint m_pTargetTex;
	static GLuint m_pPortalTex;
};