#ifndef ZLevel_h__
#define ZLevel_h__

#include "ZBase.h"

#include "TaskBase.h"
#include "ZBall.h"
#include "ZEventHandler.h"

#include "ThinkObjT.h"
#include "IRenderSystem.h"

#define CURVE_PATH "aa.cvd"

namespace Zuma
{
	class  ZCurvePath;
	class  ZConBallGroup;
	struct ZLevelInfo;

	int const MaxNumZConBallGroup = 2;

	class ZFrog : public ZObject 
		        , public ThinkObjT < ZFrog >
	        	, public ZEventHandler
	{
	public:
		ZFrog();
		ZColor getNextColor() const { return mColorNext; }
		void   setNextColor( ZColor val ) { mColorNext = val; }
		void   show( bool beS ){ mIsShow = beS; }
		bool   isShow() const { return mIsShow; }
		void   swapColor();

		void   shotBallThink( ThinkData* ptr );
		void   update( unsigned time );
		void   setFireBallSpeed( float speed ){ mFireBallSpeed = speed; }
		void   render( IRenderSystem& RDSystem );

		//public ZEventHandler
		virtual bool onEvent( int evtID , ZEventData const& data , ZEventHandler* sender );

		void   shotBall();

		float     mFireBallSpeed;
		ZColor    mColorNext;
		bool      mIsShow;

		float     mBallOffset;
		int       mFrameEye;
	};


	class ZLevel : public ZEventHandler
		         , public ThinkObjT< ZLevel >
	{
	public:
		ZLevel( ZLevelInfo& info );
		~ZLevel();
		void           restart();
		void           applyBomb( Vector2 const& pos );
		bool           isInScreenRange( Vector2 const& pos );
		void           update( long time );
		void           generateBall( ZConBallGroup& group , int numBall , int  numColor , int repet , int repetDec );
		ZConBall*      calcFrogAimBall( Vector2& aimPos );
		int            getTotalConBallNum() const;
		int            getBallGroupNum() const;
		ZConBallGroup* getBallGroup( int idx )  const { return mBallGroup[idx]; }
		ZFrog&         getFrog()       { return mFrog; }
		ZFrog const&   getFrog() const { return mFrog; }

		void           clacFrogColor( ThinkData* ptr );

		ZColor         porduceRandomUsableColor();

		float          getMoveSpeed() const { return mMoveSpeed; }
		void           setMoveSpeed( float val , bool beS = false );
		ZLevelInfo&    getInfo(){ return mLvInfo; }

		int            getCurComboGauge() const { return mCurComboGauge; }
		int            getMaxComboGauge() const { return mMaxComboGauge; }

		struct RenderParam
		{
			ZConBall* aimBall;
			Vector2     aimPos;
			Vector2     holeDir[ MaxNumZConBallGroup ];
		};
		void           render( IRenderSystem& RDSystem , RenderParam const& param );
		void           checkImportBall();
	protected:
		//ZEventHandler
		bool           onEvent( int evtID , ZEventData const& data , ZEventHandler* sender );


		void           createConGroup();

		void           updateMoveBall( long time );
		void           clearMoveBallList();


		ZLevelInfo&    mLvInfo;
		float          mMoveSpeed;
		unsigned       mColorBit;
		unsigned       mNextColorBit;
		int            mNumDestroyBall;

		int            mMaxComboGauge;
		int            mCurComboGauge;
		int            mCurRepet;

		typedef std::list< ZMoveBall* > MoveBallList;
		MoveBallList   mShootBallList;
		ZFrog          mFrog;

		ZConBallGroup* mBallGroup[ MaxNumZConBallGroup ];

		int            mNumSeqCombo;
		long           mTimePrvCombo;
		long           mTimeNextTool;
		long           mTimeBackward;
		long           mTimeAccuracy;
		long           mTimeSlow;
	};

}
#endif // ZLevel_h__
