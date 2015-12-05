#ifndef CARStage_h__3414efe3_7df4_44fb_8375_e9165753a1c3
#define CARStage_h__3414efe3_7df4_44fb_8375_e9165753a1c3

#include "StageBase.h"

#include "CARLevel.h"
#include "CARGameSetting.h"
#include "CARGameModule.h"
#include "CARDebug.h"

#include "Coroutine.h"
#include "StreamBuffer.h"

#include "CFlyHeader.h"

#include <fstream>
#include <sstream>

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
		void resquestPlaceTile( Vec2i const& pos , int rotation )
		{
			assert( mAction == ACTION_PLACE_TILE );
			GamePlaceTileData* myData = static_cast< GamePlaceTileData* >( mActionData );
			myData->resultPos = pos;
			myData->resultRotation = rotation;
			resquestInternal();
		}
		void resquestDeployActor( int index , ActorType type )
		{
			assert( mAction == ACTION_DEPLOY_ACTOR );
			GameDeployActorData* myData = static_cast< GameDeployActorData* >( mActionData );
			myData->resultIndex = index;
			myData->resultType  = type;
			resquestInternal();
		}

		void requestSelect( int index )
		{
			assert( mAction == ACTION_SELECT_ACTOR || 
				    mAction == ACTION_SELECT_MAPTILE ||
					mAction == ACTION_SELECT_ACTOR_INFO );

			GameSelectActionData* myData = static_cast< GameSelectActionData* >( mActionData );
			myData->resultIndex = index;
			resquestInternal();
		}


		void skipAction()
		{
			if ( mActionData )
			{
				mActionData->resultSkipAction = true;
				resquestInternal();
			}
		}

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

	class Renderer
	{
	public:
	};

	class LevelStage : public StageBase
		             , public IGameEventListener
	{
		typedef StageBase BaseClass;
	public:
		LevelStage(){}

		virtual bool onInit();

		void setRenderOffset( Vec2f const& a_offset );

		virtual void onEnd();

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void onRender( float dFrame );

		
		bool onEvent(int event , int id , GWidget* ui);
		bool onKey( unsigned key , bool isDown );
		bool onMouse( MouseMsg const& msg );

		void restart( bool bInit );

		void tick()
		{

		}

		void updateFrame( int frame )
		{


		}

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

	protected:


		typedef MapTile::SideNode SideNode;
		typedef MapTile::FarmNode FarmNode;

		CGameCoroutine    mCoroutine;
		GameModule        mMoudule;
		GamePlayerManager mPlayerManager;
		GameSetting       mSetting;

		void addActionWidget( GWidget* widget );
		std::vector< GWidget* > mGameActionUI;

		Vec2f  mRenderOffset;
		Vec2i  mCurMapPos;
		int    mRotation;

		int    mIdxShowFeature;

		IDirect3DSurface9* mSurfaceBufferTake;

		CFly::World*    mWorld;
		CFly::Object*   mTileShowObject;
		CFly::Scene*    mScene;
		CFly::Camera*   mCamera;
		CFly::Viewport* mViewport;
	};

}//namespace CAR

#endif // CARStage_h__3414efe3_7df4_44fb_8375_e9165753a1c3