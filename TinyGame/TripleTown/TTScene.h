#ifndef TTScene_h__
#define TTScene_h__

#include "TTLevel.h"

#include "Tween.h"

#include "DataStructure/Grid2D.h"
#include "DataStructure/TTable.h"
#include "RHI/RHICommon.h"
#include "RHI/RHICommand.h"
#include "RHI/TextureAtlas.h"
#include "Math/Matrix2.h"

#include "Renderer/RenderTransform2D.h"

class Graphics2D;

namespace TripleTown
{
	using namespace Render;
	using Math::Matrix2;

	class SimpleRenderState2D
	{
	public:
		SimpleRenderState2D()
		{
			color = LinearColor(1, 1, 1, 1);
		}
		LinearColor        color;
		Matrix4            baseTransform;
		RHITexture2DRef    texture;
		RHISamplerStateRef sampler;
		TransformStack2D   xformStack;

		RenderTransform2D const& getTransform() const { return xformStack.get(); }
		RenderTransform2D&       getTransform()       { return xformStack.get(); }

		void setupRenderState(RHICommandList& commandList)
		{
			RHISetFixedShaderPipelineState(commandList, baseTransform, LinearColor(1, 1, 1, 1), texture, sampler);
		}
	};

	namespace EItemImage
	{
		enum Type
		{
			Normal , 
			Outline ,
			Addition,
			AdditionOutline,

			COUNT,
		};
	}

	class IAssetSerializer
	{



	};

	class IAssetObject
	{
	public:
		virtual void improt();
		virtual void serialize(IAssetSerializer& serializer);
	};

	class TextureAsset : public IAssetObject
	{


	};

	template< typename T >
	struct TObjectPtr
	{

	};

	struct SpriteComponent
	{
	public:
		TObjectPtr< TextureAsset > texture;
		Vector2 pos;
	};

	class Renderer
	{




	};


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

		void  pickObjectInternal(TilePos const& tPos);

		void  repeekObject();
		void  setLastMousePos( Vec2i const& pos ){  mLastMousePos = pos; }

		void  render();
		void  render( Graphics2D& g );
		void  renderTile( Graphics2D& g , Vec2i const& pos , ObjectId id , int meta = 0 );

		void  drawQuad(Vector2 const& pos, Vector2 const& size, Vector2 const& uv1, Vector2 const& uv2);
		void  drawQuad(Vector2 const& size, Vector2 const& uvMin, Vector2 const& uvMax);
		void  drawQuadRotateUV90(Vector2 const& pos, Vector2 const& size, Vector2 const& uv1, Vector2 const& uv2);

		void  emitQuad(Vector2 const& p1, Vector2 const& p2, Vector2 const& uv1, Vector2 const& uv2);
		void  emitQuadRotateUV90(Vector2 const& pos, Vector2 const& size, Vector2 const& uv1, Vector2 const& uv2);


		void  drawImageInvTexY(RHITexture2D& texture, int w, int h);
		void  drawItem(int itemId, EItemImage::Type imageType);
		void  drawItemCenter(int itemId, EItemImage::Type imageType);
		void  drawImageByTexId(int texId);
		void  drawImageByTextId(int texId , Vector2 size );
		
		void  drawItemOutline(RHITexture2D& texture);
		bool  loadItemImageTexFile(char const* path, int itemId, EItemImage::Type imageType);

		void drawTilePartImpl( int x , int y , int dir , TerrainType tId , float const (*texCoord)[2] );

		bool loadTextureImageResource(char const* path, int textureId);
		bool loadItemImageResource( struct ItemImageLoadInfo& info);
		TilePos calcTilePos(Vec2i const& pos);

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

			struct  
			{
				bool bSpecial;
				Vector2 uv1;
				Vector2 uv2;
			} dirMap[4];
		};
		TGrid2D< TileData > mMap;
		Level*     mLevel;


		RenderTransform2D mMapTileToScreen;

		Vec2i      mMapOffset;
		Vec2i      mLastMousePos;

		TilePos    mRemovePos[ 36 ];
		int        mNumPosRemove;
		Vec2i      mPosPeek;


		typedef Tween::GroupTweener< float > Tweener;
		Tweener::IComponent* mMouseAnim;
		Tweener    mTweener; 
		float      mItemScale;

		struct CompactRect
		{
			Vector2 offset;
			Vector2 size;
		};

		struct UVBound
		{
			Vector2 min;
			Vector2 max;
		};

		struct TexImageInfo
		{
			UVBound atlasUV;
			RHITexture2DRef tex;
		};
		std::vector<TexImageInfo> mTexMap;

		struct ItemImageInfo
		{
			CompactRect     rect;
			UVBound         atlasUV;
			RHITexture2DRef tex;
		};
		ItemImageInfo    mItemImageMap[NUM_OBJ][EItemImage::COUNT];


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
		void postSetupLand() override;
		void postSetupMap() override;
		void notifyObjectAdded(TilePos const& pos, ObjectId id) override;
		void notifyObjectRemoved(TilePos const& pos, ObjectId id) override;
		void notifyObjectMerged(){}
		void notifyActorMoved(ObjectId id, TilePos const& posFrom, TilePos const& posTo) override;
		void notifyWorldRestore() override;
		void notifyStateChanged() override;
		//
		void markTileDirty(TilePos const& pos);



	};

}//namespace TripleTown

#endif // TTScene_h__
