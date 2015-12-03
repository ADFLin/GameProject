#ifndef TetrisStage_h__
#define TetrisStage_h__

#include "GameStage.h"
#include "GamePackage.h"
#include "GameMenuStage.h"

#include "TetrisAction.h"
#include "TetrisLevel.h"

class  NetWorker;
class  CLPlayerManager;
class  INetFrameHelper;

struct Record;
class  RecordManager;

namespace Tetris
{
	class  Mode;
	class  GameWorld;
	struct GameInfo;
	class  CFrameActionTemplate;

	enum TetrisAttrib
	{
		ATTR_TT_MODE = ATTR_NEXT_ID ,
	};

	class LevelStage : public GameSubStage
	{
		typedef GameSubStage BaseClass;

	public:

		enum
		{

		};
		LevelStage();
		~LevelStage();

		static Vec2i const UI_BasePos;
		static Vec2i const UI_ButtonSize;

		bool onInit();
		void onEnd();
		void onRestart( uint64 seed , bool beInit);
		void onRender( float dFrame );

		void tick();
		void updateFrame( int frame );

		void setupLocalGame( LocalPlayerManager& playerManager );
		void setupScene( IPlayerManager& playerMgr );
		bool onWidgetEvent( int event , int id , GWidget* ui );
		void onChangeState( GameState state );
		bool getAttribValue( AttribValue& value );
		bool setupAttrib( AttribValue const& value );

		bool setupGame( GameInfo &gameInfo );

		Mode*                 createMode( GameInfo const& info );
		GameWorld*            getWorld(){ return mWorld.get(); }
		CFrameActionTemplate* createActionTemplate( unsigned version );
		bool                  setupNetwork( NetWorker* netWorker , INetEngine** engine );
		TPtrHolder< GameWorld > mWorld;
		Mode*    mGameMode;
		long     mGameTime;
		int      mLastGameOrder;

		friend class CNetRoomSettingHelper;
	};


	class MenuStage : public GameMenuStage
	{
		typedef GameMenuStage BaseClass;
	public:
		enum
		{
			UI_TIME_MODE  = BaseClass::NEXT_UI_ID ,
			UI_BATTLE_MODE ,
			UI_CHALLENGE_MODE ,
			UI_PRACTICE_MODE  ,

			UI_SHOW_SCORE ,
			UI_GAME_OPTION ,
			UI_ABOUT_GAME ,
		};

		enum
		{
			eMainGroup   = 0,
		};
		bool onInit();
		void doChangeGroup( StageGroupID group );
		void onUpdate( long time );
		void onRender( float dFrame );
		bool onEvent( int event , int id , GWidget* ui );


		Vec2i  offsetBG;
		int    ix,iy;
		float  t;
	};

	class AboutGameStage : public StageBase
	{
	public:
		virtual bool onInit();
		virtual void onUpdate( long time );
		virtual void onRender( float dFrame );

		struct PieceSprite
		{
			Piece piece;
			float angle;
			float angleVel;
			Vec2f pos;
			Vec2f vel;

			void render();
			void update( float dt );
		};
		void productSprite( PieceSprite& spr );

		static int const TotalSpriteNum = 50; 
		PieceSprite sprite[ TotalSpriteNum ];
		int         curIndex;
	};


	class RecordStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		RecordStage();

		bool onEvent( int event , int id , GWidget* ui );
		bool onKey( unsigned key , bool isDown );
		bool onInit();
		void onUpdate( long time );
		void onRender( float dFrame );

		void setPlayerOrder( int order );
		void renderRecord( GWidget* ui );

		Record* lightRecord;
		int     lightBlink;
		int     lightOrder;

		int     idxChar;
		bool    chName;
	};


}//namespace Tetris

#endif // TetrisStage_h__
