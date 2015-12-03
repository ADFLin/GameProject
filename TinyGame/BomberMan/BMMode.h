#ifndef BMMode_h__
#define BMMode_h__

#include "BMLevel.h"

namespace BomberMan
{

	class IMapGenerator;
	class World;
	class Player;

	enum ModeId
	{
		MODE_BATTLE ,
	};


	class Mode : public WorldListener
	{
	public:
		Mode( ModeId id , IMapGenerator& mapGen );

		enum State
		{
			eRUN ,
			eROUND_END ,
			eGAME_OVER ,
		};

		void           setupWorld( World& world );
		void           setMapGenerator( IMapGenerator* mapGen ){ mMapGen = mapGen; }
		void           tick();
		void           restart( bool beInit );
		World&         getWorld(){ return *mLevel; }
		ModeId         getId() const { return mId;  }
		IMapGenerator& getMapGenerator(){ return *mMapGen; }
		Player*        addPlayer();
		State          getState() const { return mState; }

	protected:
		void    setState( State state ){ mState = state; }
		virtual void   onRestart( bool beInit ) = 0;
		virtual void   onTick() = 0;
		virtual bool   produceTool( int& tool ){ return false; }
	protected:

		void onBreakTileObject( Vec2i const& pos , int prevId , int prevMeta );
		void onPlayerEvent( EventId id , Player& player );

	private:
		State          mState;
		ModeId         mId;
		IMapGenerator* mMapGen;
		World*         mLevel;
	};



	class IMapGenerator
	{
	public:
		virtual void  generate( World& world , ModeId id , int playeNum , bool beInit ) = 0;
		virtual Vec2i getPlayerEntry( int idx ) = 0;
		virtual int   getMaxPlayerNum( ModeId id ) = 0;
		virtual bool  isSupportMode( ModeId id ) = 0;

		static void generateHollowTile( World& world , Vec2i const& start , Vec2i const& size , int depth , int id );
		static void generateRandTile( World& world , Vec2i const& start , Vec2i const& size , int ratio , int id );
		static void generateGapTile( World& world , Vec2i const& start , Vec2i const& size , int id );
		static void generateConRoad( World& world , Vec2i const& pos , int numCon );
		static void generateRail( World& world , Vec2i const posStart , Vec2i const posEnd , int numNode );
	};

	class CSimpleMapGen : public IMapGenerator
	{
	public:
		CSimpleMapGen( Vec2i const& size , uint16 const* map = NULL );
		void  generate( World& world , ModeId id , int playeNum , bool beInit );
		Vec2i getPlayerEntry( int idx ){ return mEntry[idx]; }
		bool  isSupportMode( ModeId id ){ return true; }
		int   getMaxPlayerNum( ModeId id ){ return 4; }

	private:
		Vec2i mEntry[ 8 ];
		uint16 const* mMapBase;
		Vec2i mMapSize;
	};

	class BattleMode : public Mode
	{
	public:
		BattleMode( IMapGenerator& mapGen );

		struct Info
		{
			Info()
			{
				power     = 1;
				numBomb   = 1;
				time      = 3 * 60 * 1000;
				numWinRounds = 3;
				decArea   = true;
			}

			int  power;
			int  numBomb;
			int  numWinRounds;
			bool decArea;
			int  time;
		};

		Info&  getInfo(){ return mInfo; }

		
		struct GameStatus
		{
			int numWin;
		};


		void onRestart( bool beInit );
		void onTick();
		void onPlayerEvent( EventId id , Player& player );

		bool produceTool( int& tool );

		int     mNumPlayerAlive;
		bool    mbIsOver;
		Player* mLastWinPlayer;
		Info    mInfo;
		long    mTime;

		GameStatus mPlayerGameStatus[ gMaxPlayerNum ]; 
	};

}//namespace BomberMan

#endif // BMMode_h__
