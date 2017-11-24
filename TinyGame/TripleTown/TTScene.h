#ifndef TTScene_h__
#define TTScene_h__

#include "TTLevel.h"
#include "TGrid2D.h"

#include "Tween.h"
#include "TVector2.h"

#include "TTable.h"
#include "DrawEngine.h"

#include <gl/GL.h>
class Graphics2D;

namespace TripleTown
{
	typedef TVector2< float > Vector2;

	class Texture
	{
	public:
		bool loadFile( char const* path );
		void bind(){ glBindTexture( GL_TEXTURE_2D , mTex ); }

		float  mScaleS;
		float  mScaleT;
	private:
		GLuint mTex;

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

	

		void setupLevel( Level& level );

		void updateFrame( int frame );

		void  peekObject( Vec2i const& pos );
		void  setLastMousePos( Vec2i const& pos ){  mLastMousePos = pos; }

		void  render();
		void  render( Graphics2D& g );
		void  renderTile( Graphics2D& g , Vec2i const& pos , ObjectId id , int meta = 0 );

		//AnimManager
		void  notifyActorMoved( ObjectId id , TilePos const& posFrom , TilePos const& posTo );
		void  postCreateLand();

		bool  loadFile( char const* name , GLuint& tex , unsigned& w , unsigned& h );
		void  drawQuad( int x , int y , int w , int h , float tx , float ty , float tw , float th );
		void  drawQuad( int w , int h , float tx , float ty , float tw , float th );

		void  drawImage( GLuint tex , int w , int h );
		void  drawImageInvTexY( GLuint tex , int w , int h );
		void  drawItem( GLuint tex );
		void  drawItemCenter( GLuint tex );

		void  drawItem( Texture& tex );
		void  drawItemOutline( GLuint tex );
		void  drawItemCenter( Texture& tex );

		bool  loadTexFile( char const* path , GLuint& tex , unsigned& w , unsigned& h );

		void drawRoadTile( int x , int y , int dir );
		void drawLakeTile( int x , int y , int dir );
		void drawTileImpl( int x , int y , int dir , TerrainType tId , float const (*texCoord)[2] );
		void drawTile( int x , int y , Tile const& tile );

		TilePos calcTilePos( Vec2i const& pos );
		void  drawQuadRotateTex90( int x , int y , int w , int h , float tx , float ty , float tw , float th );
		
		struct TileData
		{
			Vector2 pos;
			float scale;
			unsigned char con[4];
		};
		TGrid2D< TileData > mMap;
		Level*     mLevel;

		Vec2i      mMapOffset;
		Vec2i      mLastMousePos;

		TilePos        mRemovePos[ 36 ];
		int        mNumPosRemove;
		Vec2i      mPosPeek;

		typedef Tween::GroupTweener< float > Tweener;
		Tweener::IComponent* mMouseAnim;

		Tweener    mTweener; 

		float      mItemScale;

		GLuint     mTexMap[ 64 ];
		GLuint     mTexItemMap[ NUM_OBJ ][ 3 ];
		//Texture    mTexItemMap[ NUM_OBJ ][ 2 ];
	};

}//namespace TripleTown

#endif // TTScene_h__
