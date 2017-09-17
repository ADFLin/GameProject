#ifndef rcBase_h__
#define rcBase_h__

#include <cmath>
#include <cstdlib>
#include <cassert>

#include <vector>
#include <map>
#include <list>
#include <algorithm>


#include "Holder.h"
#include "Singleton.h"
#include "TCycleNumber.h"
#include "TVector2.h"

#include "ProfileSystem.h"
#include "LogSystem.h"

#include "FastDelegate/FastDelegate.h"


typedef TVector2< int > Vec2i;
typedef TCycleNumber< 8 , char > rcDirection;

class rcBuilding;
class rcHouse;
class rcWorker;
class rcLevelMap;
class rcLevelCity;

#define ARRAY_SIZE( array ) ( sizeof(array) / sizeof( array[0]) )
#ifndef BIT
#	define BIT(i) ( 1 << (i) )
#endif

typedef unsigned char  uint8;
typedef unsigned short uint16;

long const g_UpdateFrameRate = 15;

Vec2i const g_OffsetDir[] =
{
	Vec2i( 1 , 0 ) , Vec2i( 0 , 1 ) , Vec2i( -1 , 0 ) , Vec2i( 0 , -1 ),
};

struct rcGlobal
{
	long systemTime;
	long gameTime;
	long cityTime;
}; 

rcGlobal const& getGlobal();

#define RC_ERROR_BUILDING_ID (( unsigned )-1 )
#define RC_ERROR_TEXTURE_ID  (( unsigned )0 )

#endif // rcBase_h__