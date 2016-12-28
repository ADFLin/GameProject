#ifndef rcPathFinder_h__
#define rcPathFinder_h__

#include "rcBase.h"
#include <vector>

class rcLevelMap;
class rcBuilding;

class rcPath;


class rcPathFinder
{
public:
	virtual ~rcPathFinder(){}
	virtual  rcPath*  sreachPath( Vec2i const& from ) = 0;

	void   setLevelMap( rcLevelMap* levelMap ){  mLevelMap = levelMap; }
	
	void   setupDestion( Vec2i const& mapPos );
	void   setupDestion( rcBuilding* building );

	static rcPathFinder* creatPathFinder( Vec2i const& size );

protected:
	enum FindType
	{
		FT_BUILDING ,
		FT_POSITION ,
		FT_BUILDING_TAG ,
	};


	FindType    mType;
	unsigned    mBuildID;
	Vec2i       mEndPos;
	rcLevelMap* mLevelMap;

};




class rcPath
{
public:
	typedef std::list< Vec2i > MapPosList;
	typedef MapPosList::iterator Cookie;

	Vec2i const& getStartPos(){ return mRotePosList.front(); }
	Vec2i const& getGoalPos() { return mRotePosList.back(); }

	void   reverseRoute(){  mRotePosList.reverse(); }
	void   startRouting( Cookie& cookie );
	bool   getNextRoutePos( Cookie& cookie , Vec2i& pos );
	void   popBackPos(){ mRotePosList.pop_back(); }

	void   addRoutePos( Vec2i const& pos ){  mRotePosList.push_back( pos );  }
	MapPosList mRotePosList;
};

#endif // rcPathFinder_h__