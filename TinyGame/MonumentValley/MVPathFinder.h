#ifndef MVPathFinder_h__
#define MVPathFinder_h__

#include "MVCommon.h"
#include "MVWorld.h"

#include "DataStructure/Array.h"

namespace MV
{
	struct FindState
	{
		FindState()
		{
			prevBlockNode = nullptr;
		}
		Block*   block;
		NavNode* prevBlockNode;
		Dir      faceDirL;
		Dir      upDir;
		bool     bParallaxLink;
	};

	struct PathNode
	{
		int      posCount;
		Dir      frontDir;
		Dir      upDir;
	
		NavNode* link;
		Block*   block;
		Dir      faceDir;
		BlockSurface* surface;

		float    moveFactor;
		char     parallaxDir;
		int      fixPosOffset;
	};

	class Path
	{
	public:
		PathNode const& getNode( int idx ) const { return mNodes[idx]; }
		PathNode&       getNode( int idx )       { return mNodes[idx]; }
		int             getNodeNum() const { return (int)mNodes.size(); }
	private:
		friend class PathFinder;
		TArray< PathNode > mNodes;
	};

	typedef TArray< Vec3f > PointVec;

	class PathFinder
	{
	public:
		bool find( FindState const& from , FindState const& to );
		void buildPath( Path& path , PointVec& points , World const& world );
	};

	class Navigator
	{
	public:
		Navigator()
		{
			mState = eFinish;
		}

		enum State
		{
			eRun ,
			eFinish ,
			eDisconnect ,
		};


		void moveToPos( Block* block , Dir faceDir );

		void initMove();


		void moveReplay();

		void update( float dt );

		void setupNodeParam();

		void setupActorOnNode();

		void fixPathParallaxPosition( int idxStart );


		PointVec mMovePoints;
		Path     mPath;
		State    mState;

		Actor* mHost;
		World* mWorld;

		int    mIdxNode;
		Vec3f  mPrevPos;
		int    mIndexNodePos;
		int    mIdxNextPos;
		Vec3f  mMoveOffset;
		float  mMoveTime;
		float  mMoveDuration;
		
	};

}//namespace MV

#endif // MVPathFinder_h__
