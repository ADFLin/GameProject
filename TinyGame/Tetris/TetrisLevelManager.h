#ifndef TetrisLevelManager_h__
#define TetrisLevelManager_h__

#include "TetrisLevel.h"
#include "TetrisScene.h"

typedef unsigned __int64 uint64;

class  GWidget;
class  RenderCallBack;
class  IPlayerManager;
class  CLPlayerManager;
class  RecordManager;
class  SimpleController;
struct PlayerInfo;
class  GamePlayer;

namespace Tetris
{

	static float const PieceRotateSpeed = 0.1f;

	typedef SimpleController MyController;

	struct GameInfo;
	class Scene;
	class EventListener;
	class GameWorld;
	class Mode;
	class ModeData;

	typedef unsigned ModeID;

	class LevelData
	{
	public:
		LevelData();

		void          build( Mode* mode , bool bNeedScene );
		unsigned      getID() const { return id; }
		unsigned      getPlayerID() const { return mPlayerID; }
		Scene*        getScene() { return mScene; }
		Level*        getLevel() { return mLevel; }
		void          cleanup();
		ModeData*     getModeData() { return mModeData; }

		void fireAction( ActionTrigger& trigger );

	private:
		LevelState    lastState;
		Scene*        mScene;
		Level*        mLevel;
		ModeData*     mModeData;
		unsigned      mPlayerID;
		unsigned      id;
		int           idxUse;
		friend class GameWorld;
	};


	class ModeData
	{
	public:
		virtual ~ModeData(){}
		virtual void reset( Mode* mode , LevelData& data , bool beInit ) = 0;
	};

	class Mode
	{
	public:
		void  restart( bool beInit );
		long  getGameTime() const { return mGameTime; }

		enum
		{
			eReplay    = BIT(0) ,
			eNetGame   = BIT(1) ,
			eSpectator = BIT(2) ,
			eOwnServer = BIT(3) ,
		};
		GameWorld* getLevelManager() {  return mLevelManager;  }

		static int getMaxPlayerNumber( ModeID mode );

		virtual ModeID getModeID() = 0;

	public:
		virtual ModeData* createModeData(){ return NULL ; }
		virtual void fireAction( LevelData& lvData , ActionTrigger& trigger ) = 0;
		virtual void  getGameInfo( GameInfo& info ){}

	public:
		virtual void init() = 0;
		virtual void tick(){}
		virtual bool onWidgetEvent( int event , int id , GWidget* ui ){  return true;  }

		virtual void render( Scene* scene ){}
		virtual void setupScene( unsigned flag ){}
		virtual void setupSingleGame( MyController& controller ) = 0;
		virtual int  markRecord( GamePlayer* player , RecordManager& manager ){ return -1; }
		virtual bool checkOver(){ return false; }

		enum EventID
		{
			eREMOVE_LAYER ,
			eCHANGE_PIECE ,
			eMARK_PIECE   ,
			eLEVEL_OVER   ,
		};

		struct LevelEvent
		{
			int id;

			union
			{
				struct
				{
					int* layer;
					int  numLayer;
				};
			};

		};

		virtual void onLevelEvent( LevelData& lvData , LevelEvent const& event ){}
		virtual void onGameOver(){}
		virtual void renderStats( GWidget* ui ){}

	protected:
		virtual void doRestart( bool beInit ){}
		friend class GameWorld;
		long          mGameTime;
		GameWorld* mLevelManager;
	};


	class  Mode;


	class GameWorld : public Level::EventListener
		            , public ActionEnumer
		            , public UnCopiable
	{
	public:
		GameWorld( Mode* mode ){  init( mode ); }
		~GameWorld();

		void         restart( bool beInit );
		void         render( Graphics2D& g );
		void         tick();
		void         updateFrame( int frame );
		bool         removePlayer( unsigned id );

		int          getLevelNum()    { return mNumLevel; }
		int          getLevelOverNum(){ return mNumLevelOver; }
		void         storePlayerLevel( IPlayerManager& playerMgr );
		bool         isGameEnd(){  return mbGameEnd;  }

		LevelData*   createPlayerLevel( GamePlayer* player );
		LevelData*   findPlayerData( unsigned id );

		// ActionEnumer
		void         fireAction( ActionTrigger& trigger );
		void         fireLevelAction( ActionTrigger& trigger );

		Mode*        getLevelMode() { return mLevelMode; }
		void         setLevelMode( Mode* mode );
		void         setupScene( unsigned mainID , unsigned flag );

		LevelData* getLevelData( int lvID )
		{
			assert( 0 <= lvID && lvID < gTetrisMaxPlayerNum );
			if ( mDataStorage[lvID].idxUse == -1 )
				return NULL;
			return &mDataStorage[ lvID ];
		}
	private:
		//Level::EventListener
		void         onRemoveLayer( Level* level , int layer[] , int numLayer );
		void         onMarkPiece  ( Level* level , int layer[] , int numLayer );
		void         onChangePiece( Level* level );

		void         calcScenePos( unsigned mainID );
		int          getSortedLevelData( unsigned mainID , LevelData* sortedData[] );
		LevelData&   getUsingData( unsigned idx )
		{
			assert( mUsingData[idx] );
			return *mUsingData[idx];
		}
		void  init( Mode* mode );
		void  addBaseUI( LevelData& lvData , Vec2i& pos , RenderCallBack* statsCallback );

		bool  mbGameEnd;
		int   mNumLevelOver;
		int   mNumLevel;


		LevelData  mDataStorage[ gTetrisMaxPlayerNum ];
		LevelData* mUsingData[ gTetrisMaxPlayerNum ];
		Mode*      mLevelMode;
		bool       mNeedScene;

	};

}//namespace Tetris

#endif // TetrisLevelManager_h__