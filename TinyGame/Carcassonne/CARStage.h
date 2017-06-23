#ifndef CARStage_h__3414efe3_7df4_44fb_8375_e9165753a1c3
#define CARStage_h__3414efe3_7df4_44fb_8375_e9165753a1c3

#include "CFlyHeader.h"
#include "GameStage.h"

#include "CARWorldTileManager.h"
#include "CARGameSetting.h"
#include "CARGameModule.h"
#include "CARDebug.h"
#include "CARGameInputImpl.h"

#include "FixVector.h"

#include <fstream>
#include <sstream>

#define CAR_USE_INPUT_COMMAND 1

class IDataTransfer;

namespace CAR
{
	using CFly::Vector3;
	using CFly::Matrix4;

	class LevelStage;

	class ActorPosButton : public GButtonBase
	{
		typedef GButtonBase BaseClass;
	public:
		ActorPosButton( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );

		void onRender();

		ActorPosInfo* info;
		ActorType type;
		int       indexPos;
		Vec2f     mapPos;
	};

	class SelectButton : public GButtonBase
	{
		typedef GButtonBase BaseClass;
	public:
		SelectButton( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent )
			:BaseClass(id,pos,size,parent){}

		void onRender();

		union
		{
			MapTile*    mapTile;
			LevelActor* actor;
			ActorInfo*  actorInfo;
		};
		int       index;
		Vec2f     mapPos;
	};

	class TileButton : public GButtonBase
	{
		CFly::Sprite* mSprite;
	};

	class AuctionPanel : public GPanel
	{
		typedef GPanel BaseClass;

	public:
		AuctionPanel( int id , Vec2i const& pos , GWidget* parent )
			:GPanel( id , pos , Vec2i( 320 , 100 ) , parent )
		{
			mSprite = nullptr;
		}

		~AuctionPanel()
		{
			if ( mSprite )
				mSprite->release();
		}

		enum
		{
			UI_TILE_ID_SELECT = UI_WEIGET_ID ,
			UI_SCORE ,
		};

		void init( LevelStage& stage , GameAuctionTileData* data);
		void fireInput( CGameInput& input );
		void onRender();
		virtual bool onChildEvent( int event , int id , GWidget* ui );

		CFly::Sprite* mSprite;
		GameAuctionTileData* mData;
	};

	//class CarPlayer : public PlayerBase
	//{
	//public:
	//	GamePlayer* mProxyPlayer;
	//};

	class Renderer
	{
	public:
	};

	class LevelStage : public GameStageBase
		             , public IGameEventListener
	{
		typedef GameStageBase BaseClass;
	public:
		LevelStage(){}

		virtual bool onInit();
		virtual void onEnd();



		virtual void onRestart(uint64 seed, bool bInit);
		virtual void onRender( float dFrame );
		virtual bool onWidgetEvent(int event , int id , GWidget* ui);
		virtual bool onKey( unsigned key , bool isDown );
		virtual bool onMouse( MouseMsg const& msg );


		virtual bool setupNetwork( NetWorker* worker , INetEngine** engine ){ return true; }
		virtual void buildServerLevel( GameLevelInfo& info )
		{
			info.seed = 10;
		}

		virtual void setupLocalGame( LocalPlayerManager& playerManager );
		virtual void setupScene( IPlayerManager& playerManager );

		void tick();

		void updateFrame( int frame )
		{


		}

		enum 
		{
			UI_ACTOR_POS_BUTTON = BaseClass::NEXT_UI_ID,
			UI_MAP_TILE_BUTTON ,
			UI_ACTOR_BUTTON ,
			UI_ACTOR_INFO_BUTTON ,
			UI_ACTION_SKIP ,
			UI_ACTION_END_TURN ,
			UI_BUILD_CASTLE ,
			UI_BUY_AUCTION_TILE ,
			UI_AUCTION_TILE_PANEL ,

			UI_REPLAY_STOP ,

			UI_TILE_SELECT ,

			NEXT_UI_ID,
		};


		
		Vec2i showPlayerInfo( Graphics2D& g, Vec2i const& pos , PlayerBase* player , int offsetY );
		Vec2i showFeatureInfo( Graphics2D& g, Vec2i const& pos , FeatureBase* build , int offsetY );
		void  drawMapData( Graphics2D& g , Vec2f const& pos , MapTile const& mapData );
		void  drawTileRect( Graphics2D& g , Vec2f const& mapPos );
		void  drawTile( Graphics2D& g , Vec2f const& pos , TilePiece const& tile , int rotation , MapTile const* mapTile = nullptr );

		void  setRenderOffset( Vec2f const& a_offset );
		void  setRenderScale( float scale );
		Vec2f calcActorMapPos( ActorPos const& pos , MapTile const& mapTile );
		Vec2i convertToMapTilePos( Vec2i const& sPos );
		Vec2f convertToMapPos(Vec2i const& sPos);
		Vec2f convertToScreenPos(Vec2f const& pos);

		void  onGamePrevAction( GameModule& moudule , CGameInput& input );
		void  onGameAction( GameModule& moudule , CGameInput& input );
		void  removeGameActionUI();
		Vec2f getActorPosMapOffset( ActorPos const& pos );

		//IGameEventListener
		virtual void onPutTile( TileId id , MapTile* mapTiles[] , int numMapTile );
		
		//
		void setTileObjectTexture( CFly::Object* obj, TileId id , int idMesh = 0 );
		void createTileMesh( CFly::Object* obj , TileId id );
		void updateTileMesh( CFly::Object* obj , TileId newId , TileId oldId );
		void updateShowTileObject( TileId id );
		void getTileTexturePath( TileId id, FixString< 512 > &texName );

		void addActionWidget( GWidget* widget );

		bool canInput();
		bool isUserTrun();
		int  getActionPlayerId();
		void onRecvDataSV( int slot , int dataId , void* data , int dataSize );

		GameSetting& getSetting(){ return mSetting; }
		
		void cleanupGameData( bool bEndStage );


	protected:


		typedef MapTile::SideNode SideNode;
		typedef MapTile::FarmNode FarmNode;

		CGameInput        mInput;
		GameModule        mMoudule;
		GamePlayerManager mPlayerManager;
		GameSetting       mSetting;

		IDataTransfer*    mServerDataTranfser;

		std::vector< GWidget* > mGameActionUI;

		float   mRenderScale;
		Vec2f   mRenderTileSize;
		Vec2f   mRenderOffset;
		Matrix4 mMatVP;
		Matrix4 mMatInvVP;
		bool    mb2DView;


		int    mFiledTypeMap[ FieldType::NUM ];

		Vec2i  mCurMapPos;
		int    mRotation;

		int    mIdxShowFeature;

		IDirect3DSurface9* mSurfaceBufferTake;

		std::vector< CFly::SceneNode* > mRenderObjects;
		CFly::World*    mWorld;
		
		CFly::Scene*    mScene;
		CFly::Scene*    mSceneUI;
		CFly::Camera*   mCamera;
		CFly::Viewport* mViewport;

		CFly::Object*   mTileShowObject;
		TileId          mTileIdShow;

#if CAR_USE_INPUT_COMMAND
		std::vector< class IInputCommand* > mInputCommands;
		void addInputCommand(IInputCommand* command) { mInputCommands.push_back(command); }
#endif
	};

}//namespace CAR

#endif // CARStage_h__3414efe3_7df4_44fb_8375_e9165753a1c3