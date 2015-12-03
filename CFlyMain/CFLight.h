#ifndef CFLight_h__
#define CFLight_h__

#include "CFBase.h"
#include "CFSceneNode.h"

namespace CFly
{

	enum LightType
	{
		CFLT_POINT   ,
		CFLT_PARALLEL,
		CFLT_SPOT    ,
	};

	class Light : public SceneNode
	{
	public:
		Light( Scene* scene );

		LightType getLightType() const { return mType; }
		void      setLightType( LightType type );
		//void setIntensity(float){}
		
		void      setSpotCone( float iAngle , float oAngle , float falloff );

	
		Vector3        getLightDir();
		void           setLocalDir( Vector3 const& dir ){ mLocalDir = dir; }
		Vector3 const& getLightLocalDir();
		void           setLightColor( Color4f const& color );

#ifdef CF_RENDERSYSTEM_D3D9
		void    setAmbientColor( Color4f const& color ){  setColor( mD3dLight.Ambient , color ); }
		void    setDiffuseColor( Color4f const& color ){  setColor( mD3dLight.Diffuse , color ); }
		void    setSpecularColor( Color4f const& color ){  setColor( mD3dLight.Specular , color ); }
		Color4f getAmbientColor() const { return Color4f( &mD3dLight.Ambient.r ); }
		Color4f getDiffuseColor() const { return Color4f( &mD3dLight.Diffuse.r ); }
		Color4f getSpecularColor() const { return Color4f( &mD3dLight.Specular.r ); }
		float   getRange() const { return mD3dLight.Range; }
		void    setRange( float value ){ mD3dLight.Range = value; }
#else
		void    setAmbientColor( Color4f const& color ){  mColorAmbitent = color;  }
		void    setDiffuseColor( Color4f const& color ){  mColorDiffuse = color;  }
		void    setSpecularColor( Color4f const& color ){  mColorSpecular = color;  }
		Color4f const& getAmbientColor() const { return mColorAmbitent; }
		Color4f const& getDiffuseColor() const { return mColorDiffuse; }
		Color4f const& getSpecularColor() const { return mColorSpecular; }
		float   getRange() const { return mRange; }
		void    setRange( float value ){ mRange = value; }
#endif

		void  getAttenuation( float& constant, float& linear , float& quadratic );
		void  setAttenuation( float constant, float linear , float quadratic );

	private:
		//virtual 
		void onModifyTransform(  bool beLocal );

		LightType mType;
		Vector3   mLocalDir;

#ifdef CF_RENDERSYSTEM_D3D9
		D3DLIGHT9 mD3dLight;
		static void getColor( D3DCOLORVALUE const& color , float* rgba );
		static void setColor( D3DCOLORVALUE& color , float const* rgba );
#else
		Color4f   mColorAmbitent;
		Color4f   mColorDiffuse;
		Color4f   mColorSpecular;
		float     mAttenuationFactor[3];
		float     mRange;
#endif
		int       mIdxUse;
		friend class RenderSystem;
		friend class Scene;
	};

	DEFINE_ENTITY_TYPE( Light , ET_LIGHT )


}//namespace CFly

#endif // CFLight_h__