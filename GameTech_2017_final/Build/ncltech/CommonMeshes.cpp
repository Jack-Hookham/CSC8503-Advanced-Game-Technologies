#include "CommonMeshes.h"
#include <nclgl\NCLDebug.h>
#include <nclgl\OBJMesh.h>
#include <SOIL.h>

Mesh* CommonMeshes::m_pMeshes[] = { NULL };

GLuint CommonMeshes::m_pCheckerboardTex = 0;
GLuint CommonMeshes::m_pTargetTex = 0;
GLuint CommonMeshes::m_pPortalTex = 0;

void CommonMeshes::InitializeMeshes()
{
	if (m_pMeshes[0] == NULL)
	{
		//Initialise textures
		m_pCheckerboardTex = SOIL_load_OGL_texture(TEXTUREDIR"checkerboard.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_COMPRESS_TO_DXT);
		if (m_pCheckerboardTex)
		{
			glBindTexture(GL_TEXTURE_2D, m_pCheckerboardTex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //No linear interpolation to get crisp checkerboard no matter the scalling
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		else
		{
			NCLERROR("Unable to load checkerboard texture!");
		}

		m_pTargetTex = SOIL_load_OGL_texture(TEXTUREDIR"targettextureblack.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_COMPRESS_TO_DXT);
		if (m_pTargetTex)
		{
			glBindTexture(GL_TEXTURE_2D, m_pTargetTex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		else
		{
			NCLERROR("Unable to load target texture!");
		}

		m_pPortalTex = SOIL_load_OGL_texture(TEXTUREDIR"portal_cube.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_COMPRESS_TO_DXT);
		if (m_pPortalTex)
		{
			glBindTexture(GL_TEXTURE_2D, m_pPortalTex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		else
		{
			NCLERROR("Unable to load portal cube texture!");
		}

		//Initialise meshes
		m_pMeshes[DEFAULT_CUBE] = new OBJMesh(MESHDIR"cube.obj");
		m_pMeshes[DEFAULT_CUBE]->SetTexture(m_pCheckerboardTex);

		m_pMeshes[TARGET_CUBE] = new OBJMesh(MESHDIR"cube.obj");
		m_pMeshes[TARGET_CUBE]->SetTexture(m_pTargetTex);

		m_pMeshes[PORTAL_CUBE] = new OBJMesh(MESHDIR"cube.obj");
		m_pMeshes[PORTAL_CUBE]->SetTexture(m_pPortalTex);

		m_pMeshes[DEFAULT_SPHERE] = new OBJMesh(MESHDIR"sphere.obj");
		m_pMeshes[DEFAULT_SPHERE]->SetTexture(m_pCheckerboardTex);

	}
}

void CommonMeshes::ReleaseMeshes()
{
	for (size_t i = 0; i < NUM_MESHES; ++i)
	{
		SAFE_DELETE(m_pMeshes[i]);
	}

	if (m_pCheckerboardTex)
	{
		glDeleteTextures(1, &m_pCheckerboardTex);
		m_pCheckerboardTex = 0;
	}

	if (m_pTargetTex)
	{
		glDeleteTextures(1, &m_pTargetTex);
		m_pTargetTex = 0;
	}

	if (m_pPortalTex)
	{
		glDeleteTextures(1, &m_pPortalTex);
		m_pPortalTex = 0;
	}
}