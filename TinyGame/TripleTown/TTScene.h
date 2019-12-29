#ifndef TTScene_h__
#define TTScene_h__

#include "TTLevel.h"

#include "Tween.h"

#include "DataStructure/Grid2D.h"
#include "DataStructure/TTable.h"
#include "DrawEngine.h"
#include "RHI/RHICommon.h"
#include "RHI/RHICommand.h"
#include "RHI/TextureAtlas.h"
#include "Math/Matrix2.h"

#include <gl/GL.h>




class Graphics2D;

namespace TripleTown
{
	using namespace Render;
	using Math::Matrix2;


	class Transform2D
	{
	public:
		Transform2D() = default;
		Transform2D( Vector2 const& scale , float angle , Vector2 const& inOffset = Vector2::Zero())
			:M(Matrix2::ScaleThenRotate(scale , angle ) )
			,offset(inOffset)
		{
		}

		Transform2D(float angle , Vector2 const& inOffset = Vector2::Zero())
			:M(Matrix2::Rotate(angle))
			, offset(inOffset)
		{
		}

		Transform2D(Vector2 const& scale , Vector2 const& inOffset = Vector2::Zero())
			:M(Matrix2::Scale(scale))
			,offset(inOffset)
		{
		}

		static Transform2D TranslateThenScale(Vector2 const& offset, Vector2 const& scale)
		{
			return Transform2D(scale, scale * offset);
		}

		static Transform2D Identity() 
		{ 
			Transform2D result;
			result.M = Matrix2::Identity();
			result.offset = Vector2::Zero();
			return  result;
		}
		static Transform2D Translate(float x, float y)
		{
			Transform2D result;
			result.M = Matrix2::Identity();
			result.offset = Vector2(x,y);
			return  result;
		}

		static Vector2 MulOffset(Vector2 const& v, Matrix2 const& m, Vector2 const& offset)
		{
#if USE_SIMD
			__m128 rv = _mm_setr_ps(v.x, v.x, v.y, v.y);
			__m128 mv = _mm_loadu_ps(m.value);
			__m128 xv = _mm_dp_ps(rv, mv, 0x51);
			__m128 yv = _mm_dp_ps(rv, mv, 0xa2);
			__m128 resultV = _mm_add_ps( _mm_add_ps(xv,yv) , _mm_setr_ps(offset.x, offset.y, 0, 0) );
			return Vector2( resultV.m128_f32[0] , resultV.m128_f32[1] );
#else

			return v * m + offset;
#endif
		}
		Vector2 transformPosition(Vector2 const& pos) const
		{
			return MulOffset(pos, M, offset);
		}
		Vector2 transformVector(Vector2 const& v) const
		{
			return v * M;
		}
		Matrix2 M;
		Vector2 offset;

		Transform2D operator * (Transform2D const& rhs) const
		{
			//[ M  0 ] [ Mr 0 ]  =  [ M * Mr        0 ]
			//[ P  1 ] [ Pr 1 ]     [ P * Mr + Pr   1 ]
			Transform2D result;
			result.M = M * rhs.M;
			result.offset = MulOffset( offset , rhs.M , rhs.offset );
			return result;
		}

	};

	class SimpleRenderState2D
	{
	public:
		SimpleRenderState2D()
		{
			color = LinearColor(1, 1, 1, 1);
			xfromStack.push_back(Transform2D::Identity());
		}
		LinearColor    color;
		std::vector< Transform2D > xfromStack;
		RHITexture2DRef texture;
		Matrix4        projection;

		Transform2D const& getTransform() const { return xfromStack.back(); }
		Transform2D&       getTransform()       { return xfromStack.back(); }

		void multiTransform(Transform2D const& xform)
		{
			xfromStack.back() = xform * xfromStack.back();
		}

		void pushTransform(Transform2D const& xform, bool bApplyPrev = true)
		{
			if( bApplyPrev )
			{
				xfromStack.push_back( xform * xfromStack.back() );
			}
			else
			{
				xfromStack.push_back(xform);
			}
		}
		void popTransform()
		{
			xfromStack.pop_back();
		}
	};

	namespace EItemImage
	{
		enum Type
		{
			eNormal , 
			eOutline ,
			eAddition,
			eAdditionOutline,
			Count,
		};
	}


	class Scene : public LevelListener
	{
	public:
		Scene();


		~Scene();

		bool loadResource();

		void releaseResource();

		bool init();

		void click( Vec2i const& pos );

	

		ObjectId getObjectId(Vec2i const& pos);
		void setupLevel(Level& level);

		void updateFrame( int frame );

		void  peekObject( Vec2i const& pos );
		void  setLastMousePos( Vec2i const& pos ){  mLastMousePos = pos; }

		void  render();
		void  render( Graphics2D& g );
		void  renderTile( Graphics2D& g , Vec2i const& pos , ObjectId id , int meta = 0 );



		void  drawQuad(float x , float y , float w , float h , float tx , float ty , float tw , float th );
		void  drawQuad(float w , float h , float tx , float ty , float tw , float th );


		void  emitQuad(Vector2 const& p1, Vector2 const& p2, Vector2 const& uvMin, Vector2 const& uvMax);
		void  emitQuad(Vector2 const& size, Vector2 const& uvMin, Vector2 const& uvMax);

		void  drawImageInvTexY( GLuint tex , int w , int h );

		void  drawItem(int itemId, EItemImage::Type imageType);
		void  drawItemCenter(int itemId, EItemImage::Type imageType);
		void  drawItemByTexId(int texId);
		void  drawImageByTextId(int texId , Vector2 size );
		

		void  drawItemOutline( GLuint tex );

		bool  loadImageTexFile(char const* path, int itemId, EItemImage::Type imageType);

		void drawTilePartImpl( int x , int y , int dir , TerrainType tId , float const (*texCoord)[2] );

		bool loadTextureImageResource(char const* path, int textureId);
		bool loadItemImageResource( struct ItemImageLoadInfo& info);
		TilePos calcTilePos(Vec2i const& pos);
		void  drawQuadRotateTex90(float x , float y , float w , float h , float tx , float ty , float tw , float th );

		void loadPreviewTexture(char const* name);


		bool bShowPreviewTexture = false;
		bool bShowTexAtlas = false;
		struct TileData
		{
			Vector2 pos;
			Vector2 animOffset;
			float   scale;
			float   shadowScale;

			bool    bTexDirty;
			bool    bSpecial[4];
			Vector2 texPos[4];
			Vector2 texSize[4];
		};
		TGrid2D< TileData > mMap;
		Level*     mLevel;

		Vec2i      mMapOffset;
		Vec2i      mLastMousePos;


		struct CompactRect
		{
			Vector2 offset;
			Vector2 size;
		};

		TilePos    mRemovePos[ 36 ];
		int        mNumPosRemove;
		Vec2i      mPosPeek;


		typedef Tween::GroupTweener< float > Tweener;
		Tweener::IComponent* mMouseAnim;
		Tweener    mTweener; 
		float      mItemScale;

		RHITexture2DRef mTexMap[64];

		struct ItemImageInfo
		{
			CompactRect     rect;
			RHITexture2DRef texture;
		};
		ItemImageInfo    mItemImageMap[NUM_OBJ][EItemImage::Count];


		RHITexture2DRef mPreviewTexture;
		RHITexture2DRef mFontTextures[2];
		TextureAtlas    mTexAtlas;

		SimpleRenderState2D mRenderState;
		struct Vertex
		{
			Vector2 pos;
			Vector2 uv;
		};
		std::vector< Vertex > mBatchedVertices;
		void submitRenderCommand(RHICommandList& commandList);

		//ImageTexture    mTexItemMap[ NUM_OBJ ][ 2 ];

		//LevelListener
		virtual void postSetupLand() override;
		virtual void postSetupMap() override;
		virtual void notifyObjectAdded(TilePos const& pos, ObjectId id) override;
		virtual void notifyObjectRemoved(TilePos const& pos, ObjectId id) override;
		virtual void notifyActorMoved(ObjectId id, TilePos const& posFrom, TilePos const& posTo) override;

		void markTileDirty(TilePos const& pos);

	};

}//namespace TripleTown

#endif // TTScene_h__
