#ifndef CARStage_h__3414efe3_7df4_44fb_8375_e9165753a1c3
#define CARStage_h__3414efe3_7df4_44fb_8375_e9165753a1c3

#include "CFlyHeader.h"
#include "GameStage.h"

#include "CARWorldTileManager.h"
#include "CARGameplaySetting.h"
#include "CARPlayer.h"
#include "CARGameLogic.h"
#include "CARDebug.h"
#include "CARGameInputImpl.h"

#include "DataStructure/FixVector.h"

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
		using BaseClass = GButtonBase;
	public:
		ActorPosButton( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );

		void onRender() override;

		ActorPosInfo* info;
		EActor::Type  type;
		int           indexPos;
		Vector2       mapPos;
	};

	class SelectButton : public GButtonBase
	{
		using BaseClass = GButtonBase;
	public:
		SelectButton( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent )
			:BaseClass(id,pos,size,parent){}

		void onRender() override;

		union
		{
			MapTile*    mapTile;
			LevelActor* actor;
			ActorInfo*  actorInfo;
		};
		int       index;
		Vector2     mapPos;
	};

	class TileButton : public GButtonBase
	{
		CFly::Sprite* mSprite;
	};

	class AuctionPanel : public GPanel
	{
		using BaseClass = GPanel;

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
			UI_TILE_ID_SELECT = UI_WIDGET_ID ,
			UI_SCORE ,
		};

		void init( LevelStage& stage , GameAuctionTileData* data);
		void fireInput( CGameInput& input );
		void onRender() override;
		bool onChildEvent( int event , int id , GWidget* ui ) override;

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


	class NetGameRandom : public GameRandom
	{
	public:
		int getInt() override
		{
			return ::Global::RandomNet();
		}
	};
	class LevelStage : public GameStageBase
		             , public IGameEventListener
	{
		using BaseClass = GameStageBase;
	public:
		LevelStage(){}

		bool onInit() override;
		void onEnd() override;



		void onRestart(bool bInit) override;
		void onRender( float dFrame ) override;
		bool onWidgetEvent(int event , int id , GWidget* ui) override;
		bool onKey(KeyMsg const& msg) override;
		bool onMouse( MouseMsg const& msg ) override;


		bool setupNetwork( NetWorker* worker , INetEngine** engine ) override{ return true; }
		void buildServerLevel( GameLevelInfo& info ) override
		{
			info.seed = 10;
		}
		void setupLevel(GameLevelInfo const& info) override
		{
			Global::RandSeedNet(info.seed);
		}

		void setupLocalGame( LocalPlayerManager& playerManager ) override;
		void setupScene( IPlayerManager& playerManager ) override;
		
		void tick() override;

		void updateFrame( int frame ) override
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
		void  drawMapData( Graphics2D& g , Vector2 const& pos , MapTile const& mapData );
		void  drawTileRect( Graphics2D& g , Vector2 const& mapPos );
		void  drawTile( Graphics2D& g , Vector2 const& pos , TilePiece const& tile , int rotation , MapTile const* mapTile = nullptr );

		void  setRenderOffset( Vector2 const& a_offset );
		void  setRenderScale( float scale );
		Vector2 calcActorMapPos( ActorPos const& pos , MapTile const& mapTile );
		Vec2i convertToMapTilePos( Vec2i const& sPos );
		Vector2 convertToMapPos(Vec2i const& sPos);
		Vector2 convertToScreenPos(Vector2 const& posMap);

		Vector2 convertToScreenPos_2D(Vector2 const& posMap);
		Vector2 convertToScreenPos_3D(Vector2 const& posMap);

		void  onGamePrevAction( CGameInput& input );
		void  onGameAction( CGameInput& input );
		void  removeGameActionUI();
		Vector2 getActorPosMapOffset( ActorPos const& pos );

		//IGameEventListener
		void notifyPlaceTiles( TileId id , MapTile* mapTiles[] , int numMapTile ) override;
		
		//
		void setTileObjectTexture( CFly::Object* obj, TileId id , int idMesh = 0 );
		void createTileMesh( CFly::Object* obj , TileId id );
		void updateTileMesh( CFly::Object* obj , TileId newId , TileId oldId );
		void updateShowTileObject( TileId id );
		void getTileTexturePath( TileId id, InlineString< 512 > &texName );

		void addActionWidget( GWidget* widget );

		bool canInput();
		bool isUserTrun();
		int  getActionPlayerId();
		void onRecvDataSV( int slot , int dataId , void* data , int dataSize );

		GameplaySetting& getSetting(){ return mSetting; }
		
		void cleanupGameData( bool bEndStage );





	protected:


		using SideNode = MapTile::SideNode;
		using FarmNode = MapTile::FarmNode;

		CGameInput        mInput;
		GameLogic         mGameLogic;
		GamePlayerManager mPlayerManager;
		GameplaySetting   mSetting;
		NetGameRandom     mRandom;

		IDataTransfer*    mServerDataTranfser;

		std::vector< GWidget* > mGameActionUI;

		float   mRenderScale;
		Vector2 mRenderTileSize;
		Vector2 mRenderOffset;
		Matrix4 mWorldToClip;
		Matrix4 mClipToWorld;
		bool    mb2DView;


		int    mFiledTypeMap[ EField::COUNT ];

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