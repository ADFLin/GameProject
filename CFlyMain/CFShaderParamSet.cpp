#include "CFlyPCH.h"
#include "CFShaderParamSet.h"

#include "CFRenderSystem.h"
#include "CFLight.h"
#include "CFMaterial.h"
#include "CFObject.h"
#include "CFCamera.h"
#include "CFScene.h"


namespace CFly
{

	ShaderParamSet::ShaderParamSet( Scene* scene ) 
		:mScene( scene )
	{
		mSystem   = scene->_getRenderSystem();
		mDirtyBit = 0xffffffff;
	}

	void ShaderParamSet::setupGlobalParam( IShaderParamInput* input , ShaderParamGroup& paramGroup )
	{
		for ( ShaderParamList::iterator iter( paramGroup.paramList.begin() ), 
			                            itEnd( paramGroup.paramList.end() );
			iter != itEnd ; ++iter )
		{
			switch( iter->content )
			{
			case SP_WORLD:
				input->setUniform( iter->varName.c_str() , getWorldMatrix() );
				break;
			case SP_VIEW :
				input->setUniform( iter->varName.c_str() , getViewMatrix() );
				break;
			case SP_PROJ :
				input->setUniform( iter->varName.c_str() , getProjectMatrix() );
				break;
			case SP_WORLD_INV :
				input->setUniform( iter->varName.c_str() , getInvWorldMatrix() );
				break;
			case SP_VIEW_PROJ :
				input->setUniform( iter->varName.c_str() , getViewProjMatrix() );
				break;
			case SP_WVP :
				input->setUniform( iter->varName.c_str() , getWVPMatrix() );
				break;
			case SP_WORLD_VIEW:
				input->setUniform( iter->varName.c_str() , getWorldViewMatrix() );
				break;
			//case SP_AMBIENT_LIGHT:
			//	input->setUniform( iter->varName.c_str() , scene->setAmbientLight() );
			}
		}
	}

	void ShaderParamSet::setupCameraParam( IShaderParamInput* input , ShaderParamGroup& paramGroup , Camera* camera )
	{
		for ( ShaderParamList::iterator iter( paramGroup.paramList.begin() ), 
			                            itEnd( paramGroup.paramList.end() );
			iter != itEnd ; ++iter )
		{
			switch( iter->content )
			{
			case SP_CAMERA_POS :
				{
					Vector3 pos = camera->getWorldPosition();
					input->setUniform( iter->varName.c_str() , pos );
				}
				break;
			case SP_CAMERA_Z_FAR:
				input->setUniform( iter->varName.c_str() , camera->getZFar() );
				break;
			case SP_CAMERA_Z_NEAR:
				input->setUniform( iter->varName.c_str() , camera->getZNear() );
				break;
			case SP_CAMERA_VIEW:
				input->setUniform( iter->varName.c_str() , camera->getViewMatrix() );
				break;
			case SP_CAMERA_VIEW_PROJ :
			case SP_CAMERA_WORLD :
				break;
			}
		}
	}

	void ShaderParamSet::setupMaterialParam( IShaderParamInput* input , ShaderParamGroup& paramGroup , Material* material )
	{
		for ( ShaderParamList::iterator iter( paramGroup.paramList.begin() ) , 
			                            itEnd( paramGroup.paramList.end() );
			  iter != itEnd ; ++iter )
		{
			switch( iter->content )
			{
			case SP_MATERIAL_AMBIENT :
				input->setUniform( iter->varName.c_str() , material->getAmbient() );
				break;
			case SP_MATERIAL_DIFFUSE :
				input->setUniform( iter->varName.c_str() , material->getDiffuse() );
				break;
			case SP_MATERIAL_SPECULAR :
				input->setUniform( iter->varName.c_str() , material->getSpecular() );
				break;
			case SP_MATERIAL_EMISSIVE :
				input->setUniform( iter->varName.c_str() , material->getEmissive() );
				break;
			case SP_MATERIAL_SHINENESS :
				input->setUniform( iter->varName.c_str() , material->getShininess() );
				break;
			case SP_MATERIAL_TEXLAYER0 :
			case SP_MATERIAL_TEXLAYER1 :
			case SP_MATERIAL_TEXLAYER2 :
			case SP_MATERIAL_TEXLAYER3 :
			case SP_MATERIAL_TEXLAYER4 :
			case SP_MATERIAL_TEXLAYER5 :
			case SP_MATERIAL_TEXLAYER6 :
			case SP_MATERIAL_TEXLAYER7 :
				{
					int idx = iter->content - SP_MATERIAL_TEXLAYER0;
					if ( idx < material->getLayerNum() )
					{
						Texture* texture = material->getTextureLayer( idx ).getCurrentTexture();
						if ( texture )
						{
							input->setTexture( iter->varName.c_str() , texture );
						}
						else
						{

						}
					}
				}
				break;
			default:
				assert(0);
			}
		}
	}

	void ShaderParamSet::setupLightParam( IShaderParamInput* input , ShaderParamGroup& paramGroup , Light* light )
	{
		for ( ShaderParamList::iterator iter ( paramGroup.paramList.begin() ) , 
			                            itEnd( paramGroup.paramList.end() );
			iter != itEnd ; ++iter )
		{
			switch( iter->content )
			{
			case SP_LIGHT_POS:
				light->addFlag( SceneNode::NODE_WORLD_TRANS_DIRTY );
				input->setUniform( iter->varName.c_str() , light->getWorldPosition() );
				break;
			case SP_LIGHT_DIR:
				input->setUniform( iter->varName.c_str() , light->getLightDir() );
				break;
			case SP_LIGHT_AMBIENT:
				input->setUniform( iter->varName.c_str() , light->getAmbientColor() );
				break;
			case SP_LIGHT_DIFFUSE:
				input->setUniform( iter->varName.c_str() , light->getDiffuseColor() );
				break;
			case SP_LIGHT_SPECULAR:
				input->setUniform( iter->varName.c_str() , light->getSpecularColor() );
				break;
			case SP_LIGHT_RANGE:
				input->setUniform( iter->varName.c_str() , light->getRange() );
				break;
			case SP_LIGHT_ATTENUATION:
				{
					Vector3 atten;
					light->getAttenuation( atten[0] , atten[1] , atten[2] );
					input->setUniform( iter->varName.c_str() , atten );
				}
				break;
			//case SP_LIGHT_SPOT :
			}
		}
	}

	void ShaderParamSet::setupShaderParam( IShaderParamInput* input , ShaderParamGroup& paramGroup , char const* groupName )
	{
		switch( paramGroup.group )
		{
		case PG_GLOBAL :
			setupGlobalParam( input , paramGroup );
			break;
		case PG_MATERIAL :
			setupMaterialParam( input , paramGroup , mCurMaterial );
			break;
		case PG_LIGHT :
			{
				Light* light = mScene->findLightByName( groupName );
				if ( !light )
					light = mCurLight;
				setupLightParam( input , paramGroup , light );
			}
			break;
		case PG_CAMERA : 
			{
				Camera* camera = mScene->findCameraByName( groupName );
				if ( !camera )
					camera = mCurCamera;

				setupCameraParam( input , paramGroup , camera );
			}
			break;
		case PG_DEFAULT_DATA:
			break;
		}
	}

	void ShaderParamSet::setCurCamera( Camera* camera )
	{
		//if ( curCamera == camera )
		//	return;
		mCurCamera = camera;
		mDirtyBit |= DIRTY_VIEW | DIRTY_PROJECT | DIRTY_VIEW_PROJ | DIRTY_WORLD_VIEW | DIRTY_WVP;
	}

	void ShaderParamSet::setCurObject( Object* obj )
	{
		//if ( curObject == obj )
		//	return;
		mCurObject = obj;
		mDirtyBit |= DIRTY_WORLD | DIRTY_INV_WORLD | DIRTY_WORLD_VIEW | DIRTY_WVP;
	}

	Matrix4 const& ShaderParamSet::getWorldMatrix()
	{
		if ( mDirtyBit & DIRTY_WORLD )
		{
			mMatWorld = mSystem->getWorldMatrix();
			mDirtyBit &= ~DIRTY_WORLD;
		}
		return mMatWorld;
	}

	Matrix4 const& ShaderParamSet::getProjectMatrix()
	{
		if ( mDirtyBit & DIRTY_PROJECT )
		{
			mMatProj = mSystem->getProjectMatrix();
			mDirtyBit &= ~DIRTY_PROJECT;
		}
		return mMatProj;
	}

	Matrix4 const& ShaderParamSet::getViewMatrix()
	{
		if ( mDirtyBit & DIRTY_VIEW )
		{
			mView = mSystem->getViewMatrix();
			//d3dDevice->GetTransform( D3DTS_VIEW , &mView );
			mDirtyBit &= ~DIRTY_VIEW;
		}
		return mView;
	}

	Matrix4 const& ShaderParamSet::getWorldViewMatrix()
	{
		if ( mDirtyBit & DIRTY_WORLD_VIEW )
		{
			mWorldView = getWorldMatrix() * getViewMatrix();
			mDirtyBit &= ~DIRTY_WORLD_VIEW;
		}
		return mWorldView;
	}

	Matrix4 const& ShaderParamSet::getWVPMatrix()
	{
		if ( mDirtyBit & DIRTY_WVP )
		{
			mWVP = getWorldViewMatrix() * getProjectMatrix();
			mDirtyBit &= ~DIRTY_WVP;
		}
		return mWVP;
	}

	Matrix4 const& ShaderParamSet::getViewProjMatrix()
	{
		if ( mDirtyBit & DIRTY_VIEW_PROJ )
		{
			mViewProj = getViewMatrix() * getProjectMatrix();
			mDirtyBit &= ~DIRTY_VIEW_PROJ;
		}
		return mViewProj;
	}

	Matrix4 const& ShaderParamSet::getInvWorldMatrix()
	{
		if ( mDirtyBit & DIRTY_INV_WORLD )
		{
			float det;
			getWorldMatrix().inverse( mInvWorld , det );
			mDirtyBit &= ~DIRTY_INV_WORLD;
		}
		return mInvWorld;
	}

}//namespace CFly