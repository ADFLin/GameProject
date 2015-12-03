#ifndef CFTexture_h__
#define CFTexture_h__


#include "CFBase.h"

namespace CFly
{

	class MaterialManager;
	class RenderTexture;

	typedef IDirect3DCubeTexture9   D3DCubeTexture;
	typedef IDirect3DVolumeTexture9 D3DVolumeTexture;
	typedef IDirect3DSurface9 D3DSurface;

	enum TextureType
	{
		CFT_TEXTURE_1D ,
		CFT_TEXTURE_2D ,
		CFT_TEXTURE_3D ,
		CFT_TEXTURE_CUBE_MAP ,
	};



	class Texture : public Entity
		          , public RefObjectT< Texture >
	{
	public:

		enum TextureFlag
		{
			TF_USAGE_RENDER_TARGET = BIT(0),
			TF_USAGE_ZBUFFER       = BIT(1),
			TF_USAGE_COLOR_KEY     = BIT(2),
		};
		Texture( D3DTexture* d3dTexture , MaterialManager* manager );
		Texture( D3DCubeTexture* d3dTexture , MaterialManager* manager );
		Texture( D3DVolumeTexture* d3dTexture , MaterialManager* manager );
		~Texture();

		MaterialManager* manager;
		RenderTexture* getRenderTarget( uint32 face = 0 )
		{
			assert( mFlag & TF_USAGE_RENDER_TARGET );
			return mRenderTargetVec[ face ];
		}

		bool        haveColorKey() const { return checkFlag( TF_USAGE_COLOR_KEY );  }
		bool        isRenderTarget() const { return checkFlag( TF_USAGE_RENDER_TARGET ); }

		bool        checkFlag( unsigned bit ) const { return (mFlag & bit ) != 0; }
		TextureType getType(){ return mType; }

		void        getSize( unsigned& w , unsigned& h );
		void        setName( char const* name ){  mName = name; }
		String const& getName(){ return mName; }
	public:
		void        _addDepthRenderBuffer( RenderSystem* renderSystem , int w , int h );
		D3DDevice*  _getD3DDevice();
		D3DSurface* _getCubeMapSurface( UINT level , UINT face  );
		D3DSurface* _getSurface( UINT level );

		void _setupAttribute( TextureType type , unsigned flag );

#ifdef CF_RENDERSYSTEM_D3D9
		union
		{
			D3DTexture*       mTex2D;
			D3DCubeTexture*   mTexCubeMap;
			D3DVolumeTexture* mTexVolume;
		};
#else
		GLuint mIdHandle;
#endif 

		D3DTexture* getD3DTexture(){ return mTex2D; }

		TextureType mType;
		typedef RefPtrT< RenderTexture > RenderTexturePtr; 
		std::vector< RenderTexturePtr > mRenderTargetVec;
		unsigned  mipMapLevel;
		unsigned  mFlag;
		String    mName;
		friend class MaterialManager;
		void   destroyThis();
	};

}//namespace CFly

#endif // CFTexture_h__
