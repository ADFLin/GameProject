#include "CFlyPCH.h"
#include "CFLight.h"

#include "CFScene.h"

namespace CFly
{

	Light::Light( Scene* scene ) :SceneNode( scene )
	{
		ZeroMemory(&mD3dLight, sizeof(mD3dLight) );

		setLightType( CFLT_POINT );
		setAmbientColor( Color4f(0,0,0,1) );
		setDiffuseColor( Color4f( 1,1,1,1 ) );
		setSpecularColor( Color4f(1,1,1,1) );
		setAttenuation( 1.0f , 0 , 0 );
		setRange( 1000.0f );
	}


	Vector3 const& Light::getLightLocalDir()
	{
		return mLocalDir;
	}

	void Light::setSpotCone( float iAngle , float oAngle , float falloff )
	{
		mD3dLight.Theta   = iAngle;
		mD3dLight.Phi     = oAngle;
		mD3dLight.Falloff = falloff;
	}

	void Light::onModifyTransform( bool beLocal )
	{
		SceneNode::onModifyTransform( beLocal );
	}

#ifdef CF_RENDERSYSTEM_D3D9
	void Light::setColor( D3DCOLORVALUE& color , float const* rgb )
	{
		if ( rgb )
		{
			color.r = rgb[0];
			color.g = rgb[1];
			color.b = rgb[2];
		}
		else
		{
			color.r = 0;
			color.g = 0;
			color.b = 0;
		}
	}

	void Light::getColor( D3DCOLORVALUE const& color , float* rgb )
	{
		assert( rgb );
		rgb[0] = color.r ;
		rgb[1] = color.g ;
		rgb[2] = color.b ;
	}
#endif

	void Light::setLightColor( Color4f const& color )
	{
		setAmbientColor( color );
		setDiffuseColor( color );
		setSpecularColor( color );
	}

	void Light::setAttenuation( float constant, float linear , float quadratic )
	{
#ifdef CF_RENDERSYSTEM_D3D9
		mD3dLight.Attenuation0 = constant;
		mD3dLight.Attenuation1 = linear;
		mD3dLight.Attenuation2 = quadratic;
#else
		mAttenuationFactor[0] = constant;
		mAttenuationFactor[1] = linear;
		mAttenuationFactor[2] = quadratic;
#endif
	}

	void Light::getAttenuation( float& constant, float& linear , float& quadratic )
	{
#ifdef CF_RENDERSYSTEM_D3D9
		constant  = mD3dLight.Attenuation0 ;
		linear    = mD3dLight.Attenuation1 ;
		quadratic = mD3dLight.Attenuation2 ;
#else
		constant  = mAttenuationFactor[0];
		linear    = mAttenuationFactor[1];
		quadratic = mAttenuationFactor[2];
#endif
	}

	CFly::Vector3 Light::getLightDir()
	{
		return getLightLocalDir() * getWorldTransform();
	}

	void Light::setLightType( LightType type )
	{
		mType = type;
#ifdef CF_RENDERSYSTEM_D3D9
		switch ( type )
		{
		case CFLT_POINT:    mD3dLight.Type = D3DLIGHT_POINT; break;
		case CFLT_PARALLEL: mD3dLight.Type = D3DLIGHT_DIRECTIONAL; break;
		case CFLT_SPOT:     mD3dLight.Type = D3DLIGHT_SPOT; break;
		}
#endif
	}



}//namespace CFly