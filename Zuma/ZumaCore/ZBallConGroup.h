#ifndef ZBallConGroup_h__
#define ZBallConGroup_h__

#include "ZBase.h"
#include "ZEventHandler.h"
#include "ZBall.h"

#include "RefCount.h"

namespace Zuma
{
	class ZConBall;
	class ZPath;
	enum  ZColor;
	class IRenderSystem;

	typedef std::list< TRefCountPtr< ZConBall> > ZBallList;
	typedef ZBallList::iterator  ZBallNode;

	enum
	{
		UG_BACKWARDS = BIT(1),
		UG_SLOW      = BIT(2),
	};

#define  MAX_NUM_COMBOINFO_TOOL 10

	enum ToolProp;

	struct ZComboInfo
	{
		Vec2f    pos;
		ZColor   color;
		int      numBall;
		int      score;
		int      numComBoKeep;
		int      numTool;
		ToolProp tool[ MAX_NUM_COMBOINFO_TOOL ];
	};

	class ZConBallGroup : public ZEventHandler
	{
	public:
		ZConBallGroup( ZPath* _path );
		virtual bool onEvent( int evtID , ZEventData const& data , ZEventHandler* sender );

		void  setForwardSpeed( float val , bool beS );
		float getForwardSpeed() const { return mSpeedForward; }

		void  update( long time , unsigned flag );

		ZConBall*   getPrevConBall( ZConBall& ball ) const;
		ZConBall*   getNextConBall( ZConBall& ball ) const;
		ZConBall*   getLastBall() const;

		ZConBall*   getFristBall() const;

		int       calcComboNum( ZConBall& ball , ZBallNode& start , ZBallNode& end );

		//destroy [start , end]
		ZConBall* destroyBall( ZBallNode start , ZBallNode end );

		void      applyBomb( Vec2f const& pos , float radius );
		//ZConBall* destroySameColorBall( ZConBall* ball );

		ZConBall* doInsertBall( ZBallNode where ,  ZColor color );

		ZConBall* insertBall( ZConBall& ball , ZColor color , bool beNext );
		ZConBall* addBall( ZColor color );
		ZPath*    getFollowPath() const { return mFollowPath; }

		ZConBall* findNearestBall( Vec2f const& pos , float& minR2 , bool checkMask = false );
		void      render( IRenderSystem& RDSystem , bool beMask );
		void      destroyBall( ZConBall* ball );
		int       getBallNum() const { return mBallList.size(); }

		float     getFinishPathPos(){ return mPathPosFinish; }
		void      reset();
		//tMin > 0
		ZConBall* rayTest( Vec2f const& rPos , Vec2f const& rDir , float& tMin );

	private:
		bool      checkBallMask( ZConBall& ball , Vec2f const pos );
		void      updateBallGroup( long time , unsigned flag );
		void      updateBall( ZConBall* cur );
		void      updateCheckBall();
		void      updateDestroyBall();

		void       addCheckBall( ZConBall* ball );

		ZBallList  mDestroyList;
		static  int const MaxNumCheckBall = 8;
		ZConBall*  mCheckBall[ MaxNumCheckBall ];
		int        mNumCheckBall;
		float      mSpeedForward;
		float      mSpeedCur;
		float      mPathPosFinish;
		ZBallList  mBallList;
		ZPath*     mFollowPath;

	};

}//namespace Zuma

#endif // ZBallConGroup_h__
