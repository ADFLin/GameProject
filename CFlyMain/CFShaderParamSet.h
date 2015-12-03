#ifndef CFShaderParamSet_h__
#define CFShaderParamSet_h__

#include "CFBase.h"

namespace CFly
{
	class IShaderParamInput
	{
	public:
		virtual void setUniform( char const* name , float val ) = 0;
		//virtual void setUniform( char const* name , float vx , float vy ) = 0;
		virtual void setUniform( char const* name , Vector3 const& data ) = 0;
		virtual void setUniform( char const* name , Color4f const& val ) = 0;
		virtual void setUniform( char const* name , Matrix4 const& data ) = 0;

		virtual void setTexture( char const* name  ,Texture* texture ) = 0;
	};

	enum ParamContent
	{
		SP_WORLD  = 0,
		SP_VIEW  ,
		SP_PROJ  ,
		SP_WORLD_VIEW ,
		SP_WORLD_INV ,
		SP_VIEW_PROJ ,
		SP_WVP ,
		SP_AMBIENT_LIGHT ,

		SP_CAMERA_POS ,
		SP_CAMERA_VIEW_PROJ ,
		SP_CAMERA_VIEW ,
		SP_CAMERA_WORLD  ,
		SP_CAMERA_Z_FAR  ,
		SP_CAMERA_Z_NEAR ,

		SP_LIGHT_POS ,
		SP_LIGHT_DIR ,
		SP_LIGHT_AMBIENT ,
		SP_LIGHT_DIFFUSE ,
		SP_LIGHT_SPECULAR ,
		SP_LIGHT_RANGE ,
		SP_LIGHT_SPOT ,
		SP_LIGHT_ATTENUATION ,

		SP_MATERIAL_AMBIENT ,
		SP_MATERIAL_DIFFUSE ,
		SP_MATERIAL_SPECULAR,
		SP_MATERIAL_EMISSIVE ,
		SP_MATERIAL_SHINENESS,
		SP_MATERIAL_TEXLAYER0 ,
		SP_MATERIAL_TEXLAYER1 ,
		SP_MATERIAL_TEXLAYER2 ,
		SP_MATERIAL_TEXLAYER3 ,
		SP_MATERIAL_TEXLAYER4 ,
		SP_MATERIAL_TEXLAYER5 ,
		SP_MATERIAL_TEXLAYER6 ,
		SP_MATERIAL_TEXLAYER7 ,

		SP_MATRIX ,
		SP_FLOAT ,
		SP_VECTOR ,
		SP_VECTOR_ARRAY ,
		SP_MATRIX_ARRAY ,
		SP_FLOAT_ARRAY ,
		SP_INT_ARRAY ,
	};

	enum ParamGroup
	{
		PG_GLOBAL ,
		PG_CAMERA ,
		PG_MATERIAL ,
		PG_LIGHT ,
		PG_DEFAULT_DATA ,
	};


	struct ShaderParam
	{
		ParamContent content;
		void*        paramVal;
		String       varName;
	};

	typedef std::vector< ShaderParam > ShaderParamList;

	struct ShaderParamGroup
	{
		ShaderParamGroup()
		{
			group = PG_DEFAULT_DATA;
		}
		ParamGroup group;
		ShaderParamList  paramList;
	};

	class ShaderParamSet
	{
	public:
		ShaderParamSet( Scene* scene );

		void setupShaderParam( IShaderParamInput* input , ShaderParamGroup& paramGroup , char const* groupName );

		Vector3 const& getLightPos();
		Vector3 const& getLightDir();

		void setCurMaterial( Material* material )
		{
			mCurMaterial = material;
		}
		void setCurCamera( Camera* camera );
		void setCurObject( Object* obj );
		void setCurViewPort( Viewport* viewPort )
		{
			mCurViewport = viewPort;
		}
		void setCurLight( Light* light )
		{
			mCurLight = light;
		}

		void setupGlobalParam  ( IShaderParamInput* input , ShaderParamGroup& paramGroup );
		void setupMaterialParam( IShaderParamInput* input , ShaderParamGroup& paramGroup , Material* material );
		void setupLightParam   ( IShaderParamInput* input , ShaderParamGroup& paramGroup , Light* light );
		void setupCameraParam  ( IShaderParamInput* input , ShaderParamGroup& paramGroup , Camera* camera );


		Matrix4 const& getWorldMatrix();
		Matrix4 const& getInvWorldMatrix();
		Matrix4 const& getViewMatrix();
		Matrix4 const& getProjectMatrix();
		Matrix4 const& getViewProjMatrix();
		Matrix4 const& getWVPMatrix();
		Matrix4 const& getWorldViewMatrix();

		enum DirtyFlag
		{
			DIRTY_WORLD      = BIT( 1 ),
			DIRTY_VIEW       = BIT( 2 ),
			DIRTY_PROJECT    = BIT( 3 ),
			DIRTY_WORLD_VIEW = BIT( 4 ),
			DIRTY_VIEW_PROJ  = BIT( 5 ),
			DIRTY_WVP        = BIT( 6 ),
			DIRTY_INV_WORLD  = BIT( 7 ),
		};
		unsigned  mDirtyBit;

		Matrix4 mMatWorld;
		Matrix4 mInvWorld;
		Matrix4 mView;
		Matrix4 mMatProj;
		Matrix4 mViewProj;
		Matrix4 mWorldView;
		Matrix4 mWVP;


		RenderSystem* mSystem;
		Scene*        mScene;
		Light*        mCurLight;
		Camera*       mCurCamera;
		Material*     mCurMaterial;
		Object*       mCurObject;
		Viewport*     mCurViewport;
	};







}//namespace CFly

#endif // CFShaderParamSet_h__
