#ifndef CFMaterial_h__
#define CFMaterial_h__

#include "CFBase.h"
#include "CFTypeDef.h"
#include "CFViewport.h"
#include "CFShaderParamSet.h"
#include "CFTransformUtility.h"

#include "CFAnimatable.h"
#include "CFTexture.h"

namespace CFly
{
	class ShaderEffect;
	class MaterialManager;

	enum CubeMapFace
	{
		CF_FACE_X     = 0,
		CF_FACE_X_INV = 1,
		CF_FACE_Y     = 2,
		CF_FACE_Y_INV = 3,
		CF_FACE_Z     = 4,
		CF_FACE_Z_INV = 5,
	};


	class IMaterial
	{







	};


	class Texture;
	class RenderTexture : public RenderTarget
	{
	public:
		RenderTexture( Texture* texture );
		RenderTexture( Texture* texture , UINT face );
	public:
		void _addDepthBuffer( RenderSystem* renderSystem , int w , int h );
	};




	DEFINE_ENTITY_TYPE( Texture , ET_TEXTURE )



	enum TexOperator
	{
		CFT_COLOR_MAP ,
		CFT_LIGHT_MAP ,
		CFT_ENVIRONMENT_MAP ,
		CFT_BUMP_MAP ,
		CFT_USER_DEFINE_MAP ,
		CFT_UNKNOW_MAP ,
	};


	class TextureLayer
	{
	public:
		enum
		{
			TLF_ENABLE        = BIT(0),
			TLF_USE_CONSTANT  = BIT(1),
			TLF_USE_ALAPHA    = BIT(2),
			TLF_USE_TRANSFORM = BIT(3),
			TLF_USE_ANIMATION = BIT(4),
		};

		TextureLayer();

		void      setupSlotNumber( unsigned num ){  mSlot.resize( num );  }

		void      setUseAlapha( bool beAlapha );
		void      setTexOperator( TexOperator Op );

		void      setTexture( Texture* tex , int slot  = 0 );
		Texture*  getTexture( int slot = 0 ){ return mSlot[slot]; }
		Texture*  getCurrentTexture();

		void      addUsageFlag( unsigned flag ){ mUsageMask |= flag; }
		void      removeUsageFlag( unsigned flag ){ mUsageMask &= ~flag; }
		bool      isEnable() const { return ( mUsageMask & TLF_ENABLE ) != 0;  }
		size_t    getSlotNum() const { return mSlot.size(); }

		int       getCurrentSlot() const { return mCurSlot;  }
		void      setCurrentSlot( int slot ){  mCurSlot = slot;  }
		void      setUsageTexcoord( int idx ){ mIdxCoord = idx; }

		// for 2D Texture transform
		void      translate( float u , float v , TransOp op );
		void      rotate( float angle , TransOp op );

		void      setAddressMode( int idx , TexAddressMode mode );
		void      setFilterMode( FilterMode filter ){ mFilter = filter; }

	public:
		void      _setD3DTexOperation( DWORD cOp , DWORD cArg1 , DWORD cArg2 , DWORD aOp , DWORD aArg1 , DWORD aArg2 );
		Texture*  _setupDevice( int idxLayer , RenderSystem* renderSystem );
	private:
		Matrix4      mTrans;
		int            mIdxCoord;
		int            mCurSlot;
		unsigned       mUsageMask;

		FilterMode     mFilter;
		TexOperator    mTexOp;
		TexAddressMode mMode[3];

		DWORD          colorOp;
		DWORD          colorArg1;
		DWORD          colorArg2;
		DWORD          alphaOp;
		DWORD          alphaArg1;
		DWORD          alphaArg2;

		struct Slot
		{
			TRefCountPtr< Texture > texture;
			unsigned  usageFlag;
		};

		std::vector< Texture::RefCountPtr > mSlot;
	};

	class TexLayerAnimation;
	class SlotTexAnimation;
	class IUVTexAnimation;

	class IMaterialShader
	{


	};

	class IRenderTechnique
	{


	};


	class Material : public Entity
		           , public RefCountedObjectT< Material >
	{
	private:
		Material(  float const* amb = nullptr ,  float const* dif = nullptr ,  
				   float const* spe = nullptr ,  float shine = 1.0f , 
				   float const* emi = nullptr );
	public:
		~Material()
		{

		}
	
		Texture* addTexture( int slot, int idxLayer, 
							  char const* texName, TextureType type = CFT_TEXTURE_2D ,
			                  Color3ub const* colorKey = nullptr , int mipMapLevel = 0  );

		int      addSenquenceTexture(  int idxLayer, char const* texFormat , int numTex , 
			                          TextureType type = CFT_TEXTURE_2D ,
									  Color3ub const* colorKey = nullptr , int mipMapLevel = 0  );

		Texture* addTexture( int slot , int idxLayer , Texture* texture );
		Texture* addRenderTarget( int slot , int idxLayer , 
			                  char const* texName , TextureFormat format , 
							  Viewport* viewport , bool haveZBuffer  );

		Texture* addRenderCubeMap( int slot , int idxLayer ,
			                   char const* texName , TextureFormat format , 
							   int length , bool haveZBuffer );



		Texture* getTexture( int idxSlot = 0 , int idxLayer = 0)
		{
			return getTextureLayer( idxLayer ).getTexture( idxSlot );
		}

		void destroyThis();

		void setAmbient( Color4f const& color ){  mColorAmbient = color;  }
		void setDiffuse( Color4f const& color ){  mColorDiffuse = color;  }
		void setSpeclar( Color4f const& color ){  mColorSpecular = color;  }
		void setEmissive( Color4f const& color ){ mColorEmissive = color;  }

		Color4f const& getAmbient() const { return mColorAmbient; }
		Color4f const& getDiffuse() const { return mColorDiffuse; }
		Color4f const& getSpecular() const { return mColorSpecular; }
		Color4f const& getEmissive() const { return mColorEmissive; }
		
		void  setShininess( float val ){ mSpecularPower = val; }
		float getShininess() const { return mSpecularPower; }

		bool useShader(){  return mShader != nullptr; }

		TextureLayer& getTextureLayer( int idxLayer );

		TextureLayer const& getTextureLayer( int idxLayer ) const;

		IMaterialShader* getShader(){ return nullptr; }
		ShaderEffect* prepareShaderEffect();
		ShaderEffect* addShaderEffect( char const* name , char const* tech );
		void          setShader( ShaderEffect* shader ,  char const* tech )
		{



		}
		
		ShaderEffect* getShaderEffect(){ return mShader; }

		void   setTexOperator( int idxLayer , TexOperator texOp );
		void   setOpacity( float val ){ mOpacity = val;  }
		float  getOpacity(){ return mOpacity; }
		

		int    getUsageSlot( int idxLayer ) const { return getTextureLayer( idxLayer ).getCurrentSlot(); }
		void   setUsageSlot( int idxLayer , int slot ) {  getTextureLayer( idxLayer ).setCurrentSlot( slot );  }
		int    getSlotNum( int idxLayer ) const {   return getTextureLayer( idxLayer ).getSlotNum();  }
		int    getLayerNum() const { return mNumLayer; }

		void   setLightingColor( LightingComponent comp , UsageColorType type )
		{
			uint32 offset = comp * 2;
			mLightingColorUsage &= ~unsigned( 0x11 << offset );
			mLightingColorUsage |= ( type << offset );
		}
		UsageColorType getLightingColor( LightingComponent comp )
		{
			uint32 offset = comp * 2;
			return UsageColorType( (mLightingColorUsage >> offset ) & 0x11 );
		}

		IUVTexAnimation*  createUVAnimation( int idxLayer , float du , float dv );
		SlotTexAnimation* createSlotAnimation( int idxLayer );

		void   updateAnimation( float dt );

		void   setTechnique( char const* name ){ mTechniqueName = name; }
		String    mTechniqueName;

	public:
		unsigned  _getUsageVertexColorBit(){ return mLightingColorUsage; }

		void   resetTextureLayer( int numLayer );
		void   adjustTextureLayer( int idxLayer );

		static void setColor( D3DCOLORVALUE& color , float* rgba );
		static void getColor( D3DCOLORVALUE const& color , float* rgba );
		

		unsigned  mLightingColorUsage;
		TRefCountPtr<ShaderEffect> mShader;
		
		int       mMaxLayer; 
		int       mNumLayer;
		std::vector< TextureLayer > mTexLayer;
		typedef  std::list< TexLayerAnimation* > MatAnimationList;
		MatAnimationList mAnimList;

		Color4f  mColorAmbient;
		Color4f  mColorDiffuse;
		Color4f  mColorSpecular;
		Color4f  mColorEmissive;
		float    mSpecularPower;
		float    mOpacity;

		friend class MaterialManager;
		friend class RenderSystem;
		MaterialManager* mMamager;
	};

	DEFINE_ENTITY_TYPE( Material , ET_MATERIAL )



	class TexLayerAnimation : public IAnimatable
	{
	public:
		TexLayerAnimation( TextureLayer* texLayer )
			:mTexLayer( texLayer ){}

		virtual void updateAnimation( float dt ) = 0;
	protected:
		TextureLayer* mTexLayer;
	};

	class SlotTexAnimation : public TexLayerAnimation
	{
	public:
		SlotTexAnimation( TextureLayer* texLayer );

		void setFramePos( float pos ){ mFramePos = pos; }
		void setFPS( float fps ){ mFPS = fps ; }
		void updateAnimation( float dt );

	private:
		bool  mIsLoop;
		int   mNumTex;
		float mFramePos;
		float mFPS;
		Material::RefCountPtr mMaterial;
		int   mIdxLayer;
	};

	class IUVTexAnimation : public TexLayerAnimation
	{
	public:
		IUVTexAnimation( TextureLayer* texLayer , float du , float dv )
			:TexLayerAnimation( texLayer )
		{
			mDeltaU = du;
			mDeltaV = dv;
			texLayer->addUsageFlag( TextureLayer::TLF_USE_TRANSFORM | TextureLayer::TLF_USE_ANIMATION );
		}
		void   updateAnimation( float dt );
		float  getDeltaU(){ return mDeltaU; }
		float  getDeltaV(){ return mDeltaV; }
	private:
		float  mDeltaU;
		float  mDeltaV;
	};


	class ShaderEffect : public Entity
		               , public RefCountedObjectT< ShaderEffect >
					   , public IShaderParamInput
	{
	public:
		ShaderEffect( MaterialManager* manager)
			:manager( manager )
		{
		
		}
		~ShaderEffect()
		{
			mD3DEffect->Release();
		}


		typedef std::map< String , ShaderParamGroup > GpuParamContent;
		GpuParamContent gpuParamContent;

		ShaderParamGroup& getGroup( ParamContent content , char const* name  );

		void addParam( ParamContent content , char const* varName );
		void addParam( ParamContent content , char const* groupName , char const* varName );

		void loadParam( ShaderParamSet* autoParamContent );
		void destroyThis();

		class PassIterator
		{
		public:
			PassIterator(ShaderEffect* shader);
			bool hasMoreElement();

		private:
			UINT nPass;
			UINT curPass;
			ShaderEffect* shader;
		};

		void setTechnique( char const* name )
		{
			mD3DEffect->SetTechnique( name );
		}

		void setUniform( char const* name , Matrix4 const& data )
		{
			mD3DEffect->SetMatrix( name , reinterpret_cast< D3DXMATRIX const* >( &data )  );
		}

		void setUniform( char const* name , Vector3 const& data )
		{
			D3DXVECTOR4 v( data.x , data.y , data.z , 1.0f );
			mD3DEffect->SetVector( name , &v );
		}

		void setUniform( char const* name , float val )
		{
			mD3DEffect->SetFloat( name , val );
		}
		void setUniform( char const* name , Color4f const& val )
		{
			mD3DEffect->SetVector( name , reinterpret_cast< D3DXVECTOR4 const*>( &val ) );
		}

		void setTexture( char const* name  ,Texture* texture )
		{
			switch( texture->getType() )
			{
			case CFT_TEXTURE_1D:
			case CFT_TEXTURE_2D:
				mD3DEffect->SetTexture( name , texture->mTex2D );
				break;
			case CFT_TEXTURE_3D:
				mD3DEffect->SetTexture( name , texture->mTexVolume );
				break;
			case CFT_TEXTURE_CUBE_MAP:
				mD3DEffect->SetTexture( name , texture->mTexCubeMap );
				break;
			}
		}

		LPD3DXEFFECT     mD3DEffect;
		MaterialManager* manager;
	};




	DEFINE_ENTITY_TYPE( ShaderEffect , ET_SHADER )




	class MaterialManager
	{
	public:
		MaterialManager( World* world );

		Material* createMaterial( float const* amb , float const* dif ,
								  float const* spe , float shine , float const* emi );

		ShaderEffect* getShaderEffect( char const* name )
		{
			return createShaderEffect( name );
		}

		ShaderEffect*  createShaderEffect( char const* name );

		Texture* createTexture( char const* texName, TextureType type , 
							     Color3ub const* colorKey = nullptr , int mipMapLevel = 0 );

		Texture* createTexture2D( char const* texName , TextureFormat format , 
			                      uint8* rawData , int w , int h ,
			                      Color3ub const* colorKey = nullptr , int mipMapLevel = 0 );

		Texture*  createRenderCubeMap( char const* texName , TextureFormat format , int len , bool haveZBuffer );
		Texture*  createRenderTarget(  char const* texName , TextureFormat format , int w , int h , bool haveZBuffer );
		void      destoryMaterial( Material* material );
		void      destoryRenderTarget( RenderTexture* renderTarget );
		void      destoryTexture( Texture* texture );
		void      destoryShader( ShaderEffect* shader );
		
		typedef std::map< String , ShaderEffect* > ShaderMap;
		typedef std::map< char const* , Texture* > TextureMap;

		ShaderMap   mShaderMap;
		TextureMap  mTextureMap;
		World*      mWorld;
		D3DDevice*  mD3dDevice;

	};


}//namespace CFly

#endif // CFMaterial_h__
