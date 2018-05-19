#include "SelectRectBase.h"

uint32 SelectRectBase::hitAreaTest( int wPosX , int wPosY )
{
	Vector2 lPos = worldToLocalPos(Vector2(wPosX, wPosY));
	int lPosX = Math::CeilToInt(lPos.x + 0.5f);
	int lPosY = Math::CeilToInt(lPos.y + 0.5f);

	Vec2i halfSize = getSize() /2;

	uint32 mask =0;
	if ( std::abs( lPosX - halfSize.x )<=LineScope )
		mask |= SIDE_RIGHT;
	if ( std::abs( lPosX + halfSize.x)<=LineScope )
		mask |= SIDE_LEFT;
	if ( std::abs( lPosY - halfSize.y )<=LineScope )
		mask |= SIDE_DOWN;
	if ( std::abs( lPosY + halfSize.y)<=LineScope )
		mask |= SIDE_TOP;

	if( mask == 0 )
	{
		if( std::abs(lPosX) < halfSize.x && std::abs(lPosY) < halfSize.y )
			return INSIDE;
		if( std::abs(lPosX)<=LineScope  &&
			lPosY <= -halfSize.y && lPosY >= -(halfSize.y + RoteLineLength) )
			return ROTEZONE;
	}
	return mask;
}

void SelectRectBase::transToLocalPoint( int wX , int wY , int& lX , int& lY )
{ 
	int dx= wX - mCenterPos.x;
	int dy= wY - mCenterPos.y;

	lX  = int( mCacheCos*dx + mCacheSin*dy);
	lY  = int(-mCacheSin*dx + mCacheCos*dy);
}


void SelectRectBase::setRotationAngle( float angle )
{
	mRotationAngle = WrapZeroTo2PI(angle);
	mCacheCos = cos(mRotationAngle);
	mCacheSin = sin(mRotationAngle);
}

Vector2 SelectRectBase::localToWorldPos(Vector2 const& lPos)
{
	float x = mCenterPos.x + mCacheCos*lPos.x - mCacheSin*lPos.y;
	float y = mCenterPos.y + mCacheSin*lPos.x + mCacheCos*lPos.y;
	return Vector2(x, y);
}

Vector2 SelectRectBase::worldToLocalPos(Vector2 const& wPos)
{
	float dx = wPos.x - mCenterPos.x;
	float dy = wPos.y - mCenterPos.y;
	return Vector2(mCacheCos*dx + mCacheSin*dy, -mCacheSin*dx + mCacheCos*dy);
}
