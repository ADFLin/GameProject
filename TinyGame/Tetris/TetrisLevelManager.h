#ifndef TetrisLevelManager_h__
#define TetrisLevelManager_h__

#include "TetrisLevel.h"
#include "TetrisScene.h"

typedef unsigned __int64 uint64;

class  GWidget;
class  RenderCallBack;
class  IPlayerManager;
class  CLPlayerManager;

class  DefaultInputControl;
struct PlayerInfo;
class  GamePlayer;

namespace Tetris
{
	class  RecordManager;
	static float const PieceRotateSpeed = 0.1f;

	typedef DefaultInputControl CInputControl;

	struct GameInfo;
	class Scene;
	class EventListener;
	class GameWorld;
	class LevelMode;
	class ModeData;

	typedef unsigned ModeID;

	class LevelData
	{
	public:
		LevelData();

		void          build( LevelMode* mode , bool bNeedScene );
		unsigned      getId() const { return id; }
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
		virtual void reset( LevelMode* mode , LevelData& data , bool beInit ) = 0;
	};

	class LevelMode
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
		GameWorld* getWorld() {  return mWorld;  }

		static int getMaxPlayerNumber( ModeID mode );

		virtual ModeID getModeID() = 0;

	public:
		virtual ModeData* createModeData(){ return NULL ; }
		virtual void fireAction( LevelData& lvData , ActionTrigger& trigger ) = 0;
		virtual void getGameInfo( GameInfo& info ){}

	public:
		virtual void init() = 0;
		virtual void tick(){}
		virtual bool onWidgetEvent( int event , int id , GWidget* ui ){  return true;  }

		virtual void render( Scene* scene ){}
		virtual void setupScene( unsigned flag ){}
		virtual void setupSingleGame( CInputControl& inputConrol ) = 0;
		virtual int  markRecord( RecordManager& manager, GamePlayer* player ){ return -1; }
		virtual bool checkOver(){ return false; }

		enum EventID
		{
			eREMOVE_LAYER ,
			eCHANGE_PIECE ,
			eMARK_PIECE   ,
			eLEVEL_OVER   ,
		};

		struct Event
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

		virtual void onLevelEvent( LevelData& lvData , Event const& event ){}
		virtual void onGameOver(){}
		virtual void renderStats( GWidget* ui ){}

	protected:
		virtual void doRestart( bool beInit ){}
		friend class GameWorld;
		long       mGameTime;
		GameWorld* mWorld;
	};

	class GameWorld : public Level::EventListener
		            , public IActionLanucher
		            , public Noncopyable
	{
	public:
		GameWorld( LevelMode* mode ){  init( mode ); }
		~GameWorld();

		void         restart( bool beInit );
		void         render( IGraphics2D& g );
		void         tick();
		void         updateFrame( int frame );
		bool         removePlayer( unsigned id );

		int          getLevelNum()    { return mNumLevel; }
		int          getLevelOverNum(){ return mNumLevelOver; }
		void         storePlayerLevel( IPlayerManager& playerManager );
		bool         isGameEnd(){  return mbGameEnd;  }

		LevelData*   createPlayerLevel( GamePlayer* player );
		LevelData*   findPlayerData( unsigned id );

		// IActionLanucher
		void         fireAction( ActionTrigger& trigger );
		void         fireLevelAction(ActionPort port, ActionTrigger& trigger );

		LevelMode*   getLevelMode() { return mModePlaying; }
		void         setLevelMode( LevelMode* mode );
		void         setupScene( unsigned mainID , unsigned flag );

		LevelData* getLevelData( int lvID )
		{
			assert( 0 <= lvID && lvID < gTetrisMaxPlayerNum );
			if ( mDataStorage[lvID].idxUse == INDEX_NONE)
				return NULL;
			return &mDataStorage[ lvID ];
		}
	private:
		//Level::EventListener
		void         onRemoveLayer( Level* level , int layer[] , int numLayer );
		void         onMarkPiece  ( Level* level , int layer[] , int numLayer );
		void         onChangePiece( Level* level );

		void         calcScenePos( unsigned mainID );
		int          sortLevelData( unsigned mainID , LevelData* sortedData[] );
		LevelData&   getUsingData( unsigned idx )
		{
			assert( mUsingData[idx] );
			return *mUsingData[idx];
		}
		void  init( LevelMode* mode );
		void  addBaseUI( LevelData& lvData , Vec2i& pos , RenderCallBack* statsCallback );

		bool  mbGameEnd;
		int   mNumLevelOver;
		int   mNumLevel;


		LevelData  mDataStorage[ gTetrisMaxPlayerNum ];
		LevelData* mUsingData[ gTetrisMaxPlayerNum ];
		LevelMode* mModePlaying;
		bool       mbNeedScene;

	};

}//namespace Tetris

#endif // TetrisLevelManager_h__