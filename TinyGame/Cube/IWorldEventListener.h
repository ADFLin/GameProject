#ifndef IWorldEventListener_h__
#define IWorldEventListener_h__

#include "CubeBase.h"

namespace Cube
{

class IWorldEventListener
{
public:
	virtual void onModifyBlock( int bx , int by , int bz ) = 0;
};

}//namespace Cube

#endif // IWorldEventListener_h__