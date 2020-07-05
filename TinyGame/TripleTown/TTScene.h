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

		void  drawImageInvTexY(RHITexture2D& texture, int w, int h);
		void  drawItem(int itemId, EItemImage::Type imageType);
		void  drawItemCenter(int itemId, EItemImage::Type imageType);
		void  drawItemByTexId(int texId);
		void  drawImageByTextId(int texId , Vector2 size );
		
		void  drawItemOutline(RHITexture2D& texture);
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
