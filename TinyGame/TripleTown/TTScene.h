#ifndef TTScene_h__
#define TTScene_h__

#include "TTLevel.h"

#include "Tween.h"
#include "TVector2.h"

#include "DataStructure/Grid2D.h"
#include "DataStructure/TTable.h"
#include "DrawEngine.h"
#include "RHI/RHICommon.h"
#include "RHI/RHICommand.h"
#include "RHI/TextureAtlas.h"


#include <gl/GL.h>
#include "Math/SIMD.h"
#define USE_SIMD 1

class Graphics2D;

namespace TripleTown
{
	using namespace Render;

	class Matrix2
	{
	public:
		Matrix2() = default;
		Matrix2(float a0, float a1, float a2, float a3)
		{
			value[0] = a0; value[1] = a1; value[2] = a2; value[3] = a3;
		}


		static Matrix2 Identity() { return Matrix2(1,0,0,1); }
		static Matrix2 Zero() { return Matrix2(0,0,0,0); }
		static Matrix2 ScaleThenRotate(Vector2 const& scale  , float angle )
		{
			float c, s;
			Math::SinCos(angle, s, c);
			return Matrix2(scale.x * c, scale.x * s, -scale.y * s, scale.y * c);

		}

		static Matrix2 RotateThenScale(float angle, Vector2 const& scale)
		{
			float c, s;
			Math::SinCos(angle, s, c);
			return Matrix2(scale.x * c, scale.y * s, -scale.x * s, scale.y * c);

		}

		static Matrix2 Rotate(float angle)
		{
			float c, s;
			Math::SinCos(angle, s, c);
			return Matrix2( c, s, -s, c);
		}

		static Matrix2 Scale(Vector2 const& scale)
		{
			return Matrix2(scale.x, 0, 0, scale.y);
		}
		
		Matrix2 operator * (Matrix2 const& rhs) const
		{
#if USE_SIMD
			__m128 lv = _mm_loadu_ps(value);
			__m128 rv = _mm_loadu_ps(rhs.value);
			__m128 r02 = _mm_shuffle_ps(rv, rv, _MM_SHUFFLE(2, 0, 2, 0));
			__m128 r13 = _mm_shuffle_ps(rv, rv, _MM_SHUFFLE(3, 1, 3, 1));
			return Matrix2(
				_mm_dp_ps(lv, r02, 0x31).m128_f32[0],
				_mm_dp_ps(lv, r13, 0x31).m128_f32[0],
				_mm_dp_ps(lv, r02, 0xc1).m128_f32[0],
				_mm_dp_ps(lv, r13, 0xc1).m128_f32[0]);
#else	
#define MAT_MUL( v1 , v2 , idx1 , idx2 ) v1[2*idx1]*v2[idx2] + v1[2*idx1+1]*v2[idx2+2]

			return Matrix2(
				MAT_MUL(value, rhs.value, 0, 0),
				MAT_MUL(value, rhs.value, 0, 1),
				MAT_MUL(value, rhs.value, 1, 0),
				MAT_MUL(value, rhs.value, 1, 1));
#undef MAT_MUL
#endif

		}

		friend Vector2 operator * ( Vector2 const& lhs, Matrix2 const& rhs )
		{
#if USE_SIMD
			__m128 lv = _mm_setr_ps(lhs.x, lhs.x, lhs.y, lhs.y);
			__m128 mv = _mm_loadu_ps(rhs.value);
			__m128 xv = _mm_dp_ps(lv, mv, 0x51);
			__m128 yv = _mm_dp_ps(lv, mv, 0xa1);
			return Vector2(xv.m128_f32[0], yv.m128_f32[0]);

#else	
			return Vector2(lhs[0] * rhs.m[0][0] + lhs[1] * rhs.m[1][0],
						   lhs[0] * rhs.m[0][1] + lhs[1] * rhs.m[1][1]);
#endif
		}

		friend Vector2 operator * (Matrix2 const& lhs, Vector2 const& rhs)
		{
#if USE_SIMD
			__m128 rv = _mm_setr_ps(rhs.x, rhs.y, rhs.x, rhs.y);
			__m128 mv = _mm_loadu_ps(lhs.value);
			__m128 xv = _mm_dp_ps(mv, rv, 0x31);
			__m128 yv = _mm_dp_ps(mv, rv, 0xc1);
			return Vector2(xv.m128_f32[0], yv.m128_f32[0]);
#else	
			return Vector2(lhs.m[0][0] * rhs[0] + lhs.m[0][1] * rhs[1],
						   lhs.m[1][0] * rhs[0] + lhs.m[1][1] * rhs[1]);
#endif
		}
		union 
		{
			float   m[2][2];
			float   value[4];
		};
	};


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
