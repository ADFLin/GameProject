#ifndef CARStage_h__3414efe3_7df4_44fb_8375_e9165753a1c3
#define CARStage_h__3414efe3_7df4_44fb_8375_e9165753a1c3

#include "CFlyHeader.h"
#include "GameStage.h"

#include "CARLevel.h"
#include "CARGameSetting.h"
#include "CARGameModule.h"
#include "CARDebug.h"

#include "Coroutine.h"
#include "StreamBuffer.h"

#include "DataTransfer.h"



#include <fstream>
#include <sstream>

class IDataTransfer;

namespace CAR
{
	enum PlayerAction
	{
		ACTION_NONE ,
		ACTION_PLACE_TILE ,
		ACTION_DEPLOY_ACTOR ,
		ACTION_SELECT_MAPTILE ,
		ACTION_SELECT_ACTOR ,
		ACTION_SELECT_ACTOR_INFO ,
	};

	
	struct SkipCom
	{
		uint8 action;
	};

	struct ActionCom
	{
		ActionCom()
		{
			numParam = 0;
		}
		void addParam( int value ){ assert( numParam < MaxParamNum ); params[ numParam++ ].iValue = value; }
		void addParam( float value ){ assert( numParam < MaxParamNum ); params[ numParam++ ].fValue = value; }
		void send( int slot , int dataId  , IDataTransfer& transfer )
		{
			transfer.sendData( slot , dataId , this , sizeof( action ) + sizeof( numParam ) + numParam * sizeof( Param ) );
		}
		static int const MaxParamNum = 16;
		uint8 action;
		uint8 numParam;
		struct Param
		{
			int   iValue;
			float fValue;
		};
		Param params[ MaxParamNum ];
	};

	enum
	{
		DATA2ID( SkipCom ) ,
		DATA2ID( ActionCom ) ,
	};
	class CGameCoroutine : public IGameCoroutine
	{
		typedef boost::coroutines::asymmetric_coroutine< void >::pull_type ImplType;
		typedef boost::coroutines::asymmetric_coroutine< void >::push_type YeildType;

	public:
		CGameCoroutine()
		{
			beRecoredMode = false;
			mAction = ACTION_NONE;
			mActionData = nullptr;
			mDataTransfer = nullptr;
		}

		void clearAction()
		{
			mAction = ACTION_NONE;
			mActionData = nullptr;
			save( "car_record" );
		}
		template< class Fun >
		void run( Fun fun )
		{
			mImpl = ImplType( std::bind( &CGameCoroutine::execEntry< Fun > , this , std::placeholders::_1 , fun ) );
		}

		void setDataTransfer( IDataTransfer* transfer );

		void sendCommond( ActionCom& com )
		{
			com.action = getAction();
			com.send( SLOT_SERVER , DATA2ID(ActionCom) , *mDataTransfer );
		}

		void onRecvCommon( int slot , int dataId , void* data , int dataSize );

		void requestCommon( ActionCom const& com );


		void resquestPlaceTile( Vec2i const& pos , int rotation );
		void resquestDeployActor( int index , ActorType type );
		void requestSelect( int index );

		void skipAction();

		void doResquestPlaceTile( Vec2i const& pos , int rotation );
		void doResquestDeployActor( int index , ActorType type );
		void doRequestSelect( int index );
		void doSkipAction();

		void returnGame()
		{
			if ( mYeild )
				mImpl();
		}

		void exitGame()
		{
			if ( mActionData && mYeild)
			{
				mActionData->resultExitGame = true;
				mImpl();
			}
		}
		PlayerAction getAction(){ return mAction; }
		GameActionData* getActionData(){ return mActionData; }

		std::function< void ( GameModule& , PlayerAction , GameActionData* ) > onAction;

		
	private:

		virtual void waitPlaceTile( GameModule& module , GamePlaceTileData& data )
		{  waitActionImpl( module , ACTION_PLACE_TILE , data ); }
		virtual void waitDeployActor( GameModule& module , GameDeployActorData& data)
		{  waitActionImpl( module , ACTION_DEPLOY_ACTOR , data ); }
		virtual void waitSelectActor(GameModule& module , GameSelectActorData& data)
		{  waitActionImpl( module , ACTION_SELECT_ACTOR , data );  }
		virtual void waitSelectActorInfo(GameModule& module , GameSelectActorInfoData& data)
		{  waitActionImpl( module , ACTION_SELECT_ACTOR_INFO , data );  }
		virtual void waitSelectMapTile(GameModule& module , GameSelectMapTileData& data)
		{  waitActionImpl( module , ACTION_SELECT_MAPTILE , data );  }
		virtual void waitTurnOver( GameModule& moudule , GameActionData& data );

		void waitActionImpl( GameModule& module , PlayerAction action , GameActionData& data );

		void resquestInternal();

		template< class Fun >
		void execEntry( YeildType& type , Fun fun )
		{
			try {
				mYeild = &type;
				fun( *this );
				mYeild = nullptr;
			} catch(const boost::coroutines::detail::forced_unwind&) 
			{
				CAR_LOG("Catch forced_unwind Exception!");
				throw;
			} catch(...) {
				// possibly not re-throw pending exception
			}
		}
	public:

		bool load( char const* file );

		bool save( char const* file );

		
		IDataTransfer*  mDataTransfer;


		bool beRecoredMode;
		std::stringstream mOFS;
		std::ifstream     mIFS;

		PlayerAction    mAction;
		GameActionData* mActionData;
		ImplType        mImpl;
		YeildType*      mYeild;
	};

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

	//class CarPlayer : public PlayerBase
	//{
	//public:
	//	GamePlayer* mProxyPlayer;
	//};

	class Renderer
	{
	public:
	};

	class LevelStage : public GameSubStage
		             , public IGameEventListener
	{
		typedef GameSubStage BaseClass;
	public:
		LevelStage(){}

		virtual bool onInit();
		virtual void onEnd();
		virtual void onRestart( uint64 seed , bool bInit);
		virtual void onRender( float dFrame );
		virtual bool onWidgetEvent(int event , int id , GWidget* ui);
		virtual bool onKey( unsigned key , bool isDown );
		virtual bool onMouse( MouseMsg const& msg );

		bool canInput()
		{
			if ( getGameType() == GT_SINGLE_GAME )
				return true;
			if ( getGameType() == GT_NET_GAME && isUserTrun() )
				return true;
			return false;
		}
		bool isUserTrun();

		virtual bool setupNetwork( NetWorker* worker , INetEngine** engine ){ return true; }
		virtual void buildServerLevel( GameLevelInfo& info )
		{
			info.seed = 0;
		}

		virtual void setupLocalGame( LocalPlayerManager& playerManager );
		virtual void setupScene( IPlayerManager& playerManager );

		void tick()
		{

		}

		void updateFrame( int frame )
		{


		}

		void setRenderOffset( Vec2f const& a_offset );
		Vec2i showPlayerInfo( Graphics2D& g, Vec2i const& pos , PlayerBase* player , int offsetY );
		Vec2i showFeatureInfo( Graphics2D& g, Vec2i const& pos , FeatureBase* build , int offsetY );
		void drawMapData( Graphics2D& g , Vec2f const& pos , MapTile const& mapData );
		void drawTile( Graphics2D& g , Vec2f const& pos , Tile const& tile , int rotation );

		Vec2f calcActorMapPos( ActorPos const& pos , MapTile const& mapTile );

		Vec2i convertToMapPos( Vec2i const& sPos );

		enum 
		{
			UI_ACTOR_POS_BUTTON = BaseClass::NEXT_UI_ID,
			UI_MAP_TILE_BUTTON ,
			UI_ACTOR_BUTTON ,
			UI_ACTOR_INFO_BUTTON ,
			UI_ACTION_SKIP ,
			UI_ACTION_END_TURN ,

			NEXT_UI_ID,
		};

		void onGameAction( GameModule& moudule , PlayerAction action , GameActionData* data );

		void removeGamePlayUI()
		{
			for( int i = 0 ; i < mGameActionUI.size() ; ++i )
			{
				mGameActionUI[i]->destroy();
			}
			mGameActionUI.clear();
		}
		Vec2f calcRenderPos(Vec2f const& pos);
		Vec2f getActorPosMapOffset( ActorPos const& pos );
		virtual void onPutTile( MapTile& mapTile );

		void setTileObjectTexture(CFly::Object* obj, TileId id );

		CFly::Object* createTileObject();

		void addActionWidget( GWidget* widget );
		void onRecvDataSV( int slot , int dataId , void* data , int dataSize );
	protected:


		typedef MapTile::SideNode SideNode;
		typedef MapTile::FarmNode FarmNode;

		CGameCoroutine    mCoroutine;
		GameModule        mMoudule;
		GamePlayerManager mPlayerManager;
		GameSetting       mSetting;

		IDataTransfer*    mServerDataTranfser;

		std::vector< GWidget* > mGameActionUI;

		Vec2f  mRenderOffset;
		Vec2i  mCurMapPos;
		int    mRotation;

		int    mIdxShowFeature;

		IDirect3DSurface9* mSurfaceBufferTake;

		std::vector< CFly::SceneNode* > mRenderObjects;
		CFly::World*    mWorld;
		CFly::Object*   mTileShowObject;
		CFly::Scene*    mScene;
		CFly::Camera*   mCamera;
		CFly::Viewport* mViewport;
	};

}//namespace CAR

#endif // CARStage_h__3414efe3_7df4_44fb_8375_e9165753a1c3