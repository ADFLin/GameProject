#ifndef IRenderSystem_h__
#define IRenderSystem_h__

#include "ZBase.h"
#include "RefCount.h"
#include "ZResManager.h"
#include "ZBall.h"

#include "RefCount.h"

struct GIFImageInfo;

namespace Zuma
{
	class ZFont;
	struct ZFontLayer;

	class ITexture2D : public ImageRes
		             , public RefCountedObjectT< ITexture2D >
		               
	{
	public:
		int  getWidth()  const { return mWidth; }
		int  getHeight() const { return mHeight; }
		bool ownAlpha()  const { return mOwnAlpha; }

		virtual void release() = 0;
	protected:
		int  mWidth;
		int  mHeight;
		bool mOwnAlpha;
	};

	enum
	{
		TBF_DONT_USE_TEX_ALPHA  = BIT(0),
		TBF_ENABLE_ALPHA_BLEND   = BIT(1),
		TBF_OFFSET_CENTER        = BIT(2),
		TBF_USE_BLEND_PARAM      = BIT(3),
	};

	enum BlendEnum
	{
		BLEND_ONE ,
		BLEND_ZERO ,
		BLEND_SRCALPHA ,
		BLEND_DSTALPHA ,
		BLEND_INV_SRCALPHA ,
		BLEND_INV_DSTALPHA ,
		BLEND_SRCCOLOR ,
		BLEND_DSTCOLOR ,
		BLEND_INV_SRCCOLOR ,
		BLEND_INV_DSTCOLOR ,
	};

	enum RenderOrder
	{
		RO_MIN_ORDER  = -1 ,
		RO_UI         = 10 ,
	};

	class IRenderSystem;
	class IRenderable
	{
	public:
		IRenderable()
			:mOrder(0)
		{
#ifdef _DEBUG
			mbManaged = false;
#endif
		}
		virtual void onRender( IRenderSystem& RDSystem ) = 0;
		virtual void setTransform( IRenderSystem& RDSystem ){}

		void setOrder( int order ){ assert( !mbManaged ); mOrder = order; }
		int  getOrder() const { return mOrder;  }
	private:
		friend class IRenderSystem;
#ifdef _DEBUG
		bool mbManaged;
#endif
		int  mOrder;
	};

	class IRenderEffect : public RefCountedObjectT< IRenderEffect >
	{
	public:
		virtual ~IRenderEffect(){}
		virtual void preRenderEffect( IRenderSystem& RDSystem ) = 0;
		virtual void postRenderEffect( IRenderSystem& RDSystem ) = 0;

	private:
		TRefCountPtr< IRenderEffect > nextEffect;
		friend class IRenderSystem;
	};

	class IMesh
	{

	};

	class MeshBulider
	{
		MeshBulider( IMesh& mesh ):mesh(mesh){}
		void setColor();
		void setPosition();
		void render( IRenderSystem& RDSystem );
		void increaseVertex();
		IMesh& mesh;
	};


	class IRenderSystem : public ResLoader
	{
	public:
		IRenderSystem();
		virtual ~IRenderSystem(){}



		virtual void  setColor( uint8 r , uint8 g , uint8 b , uint8 a = 255 ) = 0;
		//virtual void  drawBitmap( ITexture2D& tex , Vec2D const& texPos , Vec2D const& texSize ,
		//							ITexture2D& mask , Vec2D const& maskPos , Vec2D const& maskSize ,
		//                          unsigned flag );
		//virtual void  drawBitmap( ITexture2D& tex , ITexture2D& mask , unsigned flag );
		virtual void  drawBitmap( ITexture2D const& tex , unsigned flag = 0 ) = 0;
		virtual void  drawBitmap( ITexture2D const& tex , Vector2 const& texPos , Vector2 const& texSize, unsigned flag = 0 ) = 0;
		virtual void  drawBitmapWithinMask( ITexture2D const& tex , ITexture2D const& mask , Vector2 const& pos , unsigned flag = 0 ) = 0;
		virtual void  drawPolygon( Vector2 const pos[] , int num ) = 0;

		virtual void  loadWorldMatrix( Vector2 const& pos , Vector2 const& dir ) = 0;
		virtual void  translateWorld( float x , float y ) = 0;

		virtual void  rotateWorld( float angle ) = 0;
		virtual void  scaleWorld( float sx , float sy ) = 0;
		virtual void  setWorldIdentity() = 0;
		virtual void  pushWorldTransform() = 0;
		virtual void  popWorldTransform() = 0;

		virtual bool  loadTexture( ITexture2D& texture , char const* imagePath , char const* alphaPath ) = 0;
		virtual ITexture2D* createFontTexture( ZFontLayer& layer ) = 0;
		virtual ITexture2D* createTextureRes( ImageResInfo& imgInfo ) = 0;
		virtual ITexture2D* createEmptyTexture() = 0;

		virtual void prevLoadResource(){}
		virtual void postLoadResource(){}
	protected:
		
		virtual bool prevRender(){ return true; }
		virtual void postRender(){}

		virtual void  enableBlendImpl( bool beE ) = 0;
		virtual void  setupBlendFun( BlendEnum src , BlendEnum dst ) = 0;
	public:
		ITexture2D*   getTexture( ResID id );
		ZFont*        getFontRes( ResID id );
		void          setColor( ColorKey const& key ){ setColor( key.r , key.g , key.b , key.a ); }
		void          translateWorld( Vector2 const& offset ){ translateWorld( offset.x , offset.y ); }
		void          scaleWorld    ( Vector2 const& s )     { scaleWorld( s.x , s.y ); }


		void          enableBlend( bool beE );
		void          setBlendFun( BlendEnum src , BlendEnum dst );

	protected:
		//ResLoader
		ResBase*      createResource( ResID id , ResInfo& info );
		ZFont*        createFontRes( FontResInfo& info );
		bool          setupBlend( bool beOwned , unsigned flag );

		BlendEnum mSrcBlend;
		BlendEnum mDstBlend;
		bool      mBlendEnable;

	private:
		typedef TRefCountPtr< IRenderEffect > REPtr;
		struct RNODE
		{
			REPtr           effect;
			IRenderable*    obj;
		};

		typedef std::list< RNODE > RenderableList;
	public:
		typedef RenderableList::iterator RenderNode;

		IRenderable*  getRenderable( RenderNode node );
		RenderNode    addRenderable( IRenderable* obj );
		bool          removeObj( IRenderable* obj );
		bool          addEffect( RenderNode node , IRenderEffect* effect );
		bool          findRenderNode( IRenderable* obj , RenderNode& node );
		void          removeEffect( RenderNode node ,IRenderEffect* effect = NULL  );
		void          setCurOrder( int order );

		bool          beginRender();
		void          endRender();


	private:
		void   renderNode( RNODE& node );

		struct NodeCmp
		{
			bool operator()( RNODE const& n1 , RNODE const& n2 )
			{
				return n1.obj->getOrder() < n2.obj->getOrder();
			}
		};

		struct FindObj
		{
			FindObj( IRenderable* _obj ):obj(_obj){}
			bool operator()( RNODE& node ){ return node.obj == obj; }
			IRenderable* obj;
		};

		int            mCurOrder;
		RenderNode     mNextRenderNode;
		RenderableList mRenderList;
	};


	struct TempRawData
	{
		TempRawData() { buffer = NULL; }
		~TempRawData(){ delete[] buffer; }
		unsigned char* buffer;
		int       w , h;

		static bool GIFCreateAlpha( GIFImageInfo const& info , void* userData );
		static bool GIFCreateRGB( GIFImageInfo const& info , void* userData );
		static bool GIFFillAlpha( GIFImageInfo const& info , void* userData );

	};

}//namespace Zuma

#endif // IRenderSystem_h__
