#ifndef ZBall_h__
#define ZBall_h__

#include "ZBase.h"
#include "RefCount.h"
#include "DataStructure/TLinkable.h"

#include <list>

#define ZUSE_LINKABLE 0

namespace Zuma
{
	class ZEventHandler;

	extern float const g_ZBallRaidus;
	extern float const g_ZBallConDist;

	class ZObject
	{
	public:
		ZObject( ZColor color = zRed );

		ZColor        getColor() const { return mColor; }
		Vector2 const&  getPos()   const { return mPos; }
		Vector2 const&  getDir()   const { return mDir; }
		void          setColor( ZColor color ){ mColor = color; }
		void          setPos(Vector2 const& val) { mPos = val; }
		void          setDir(Vector2 const& val) { mDir = val; }

	protected:
		Vector2   mDir;
		Vector2   mPos;
		ZColor  mColor;
	};


	class ZMoveBall : public ZObject
	{
	public:
		ZMoveBall( ZColor color );

		float  getSpeed() const { return mSpeed; }
		void   setSpeed( float val) { mSpeed = val; }
		void   update( unsigned time );

	protected:
		float  mSpeed;
	};


	enum  ConState
	{
		CS_NORMAL     ,
		CS_DISCONNECT ,
		CS_INSERTING  ,
	};


	enum ToolProp
	{
		TOOL_NORMAL = 0,
		TOOL_BOMB ,
		TOOL_BACKWARDS ,
		TOOL_SLOW ,
		TOOL_ACCURACY ,

		TOOL_NUM_TOOLS ,
	};

	class   ZConBall;
	class   ZPath;
	typedef std::list< TRefCountPtr< ZConBall> > ZBallList;

#if ZUSE_LINKABLE
	typedef ZConBall* ZBallNode;
#else
	typedef ZBallList::iterator  ZBallNode;
#endif

#ifdef _DEBUG
	extern ZConBall* g_debugBall;
#endif

	enum
	{
		CBF_DESTORY    = BIT(0),
		CBF_MASK_FRONT = BIT(1),
		CBF_MASK_BACK  = BIT(2),
		CBF_INSERT_NEXT= BIT(3),
		CBF_BOMB       = BIT(4),
		CBF_MASK = CBF_MASK_BACK | CBF_MASK_FRONT ,
	};




	class ZConBall : public ZObject
		           , public RefCountedObjectT< ZConBall >
#ifdef ZUSE_LINKABLE
		           , public TLinkable< ZConBall >
#endif
	{
	public:
		ZConBall( ZColor color );

		ConState   getConState(){ return mConState; }
		void       setConState( ConState state ){ mConState = state; }

		struct UpdateInfo
		{
			ZPath*    path;
			ZConBall* prevBall;
			ZConBall* nextBall;
			float     moveSpeed;
			ZEventHandler* handler;
		};
		void       update( long time , UpdateInfo const& info );
		void       updatePathPos( float pos , ZPath& path );

		float      getPathPos(){ return mPathPos; }
		void       setPathPos( float pos ){  mPathPos = pos;  }

		ToolProp   getToolProp() const { return mToolProp; }
		void       setToolProp( ToolProp val) { mToolProp = val; }
		unsigned   getLifeTime() const { return mLifeTime; }
		float      getPathPosOffset() const { return mPathPosOffset; }

		bool       isMask() const { return checkFlag( CBF_MASK );  }

		void       addFlag( unsigned bit ){  mFlag |= bit; }
		void       removeFlag( unsigned bit ){  mFlag &= ~bit; }
		bool       checkFlag( unsigned bit ) const { return ( mFlag & bit ) != 0; }
		void       enableFlag( unsigned bit , bool beE ){ if ( beE ) addFlag( bit ); else removeFlag( bit );  }

		void       disconnect();
		void       insert();

		ZBallNode  getLinkNode();
	protected:

		int             mTimerCon;
		unsigned        mLifeTime;
		ConState        mConState;

		float           mPathPosOffset;
		float           mPathPos;
		ToolProp        mToolProp;
		bool            mIsMask;
		unsigned        mFlag;

#if ZUSE_LINKABLE

#else
		friend class  ZConBallGroup;
		ZBallNode       mNodeIter;
#endif

	};

}//namespace Zuma

#endif // ZBall_h__
