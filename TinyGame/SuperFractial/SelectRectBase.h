#ifndef SelectRectBase_h__
#define SelectRectBase_h__

#include <cstdlib>
#include <cmath>

#include "Math/TVector2.h"
#ifdef max
#undef max
#endif

#include <algorithm>



inline float WrapZeroTo2PI(float angle)
{
	return angle - (2 * PI) * Math::Floor( angle * ( 1 / (2 * PI) ) );
}

class SelectRectBase
{
public:

	typedef TVector2<int> Vec2i;
	enum
	{
		OUTSIDE    = 0x000,
		INSIDE     = 0x001,
		ROTEZONE   = 0x002,
		SIDE       = 0x100,
		SIDE_TOP   = 0x110,
		SIDE_DOWN  = 0x120,
		SIDE_LEFT  = 0x140,
		SIDE_RIGHT = 0x180,
		CORNER_RT  = SIDE_RIGHT|SIDE_TOP,
		CORNER_RD  = SIDE_RIGHT|SIDE_DOWN,
		CORNER_LT  = SIDE_LEFT|SIDE_TOP,
		CORNER_LD  = SIDE_LEFT|SIDE_DOWN,
	};

	Vec2i  getCenterPos() const { return mCenterPos; }
	Vec2i  getSize() const { return mSize; }

	int    getWidth() const { return  mSize.x; }
	int    getHeight() const { return mSize.y; }
	int    getHitArea() const{ return mHitArea; }
	float  getRotationAngle() const { return mRotationAngle; }


	void   setRotationAngle( float angle );

	Vector2 localToWorldPos(Vector2 const& lPos);
	Vector2 worldToLocalPos(Vector2 const& wPos);

	void   transToLocalPoint( int wX , int wY , int& lX , int& lY );
	uint32 hitAreaTest( int wPosX , int wPosY );

protected:

	static int const LineScope = 3;
	static int const RoteLineLength = 20;

	void setWidth(int w) {  mSize.x  = std::max( 2 , std::abs(w) );  }
	void setHeight(int h){  mSize.y = std::max( 2 , std::abs(h) );  }
	
	Vec2i   mCenterPos;
	Vec2i   mSize;
	uint32  mHitArea;
	float   mRotationAngle;
	float   mCacheCos;
	float   mCacheSin;
};

template < class T >
class SelectRectOperationT : public SelectRectBase
{
	T* _this(){ return static_cast< T*>( this ); }
	//overload function
public:
	void  enable( bool bE )
	{
		mbEnable = bE;
		_this()->onEnable(bE);
	}

	void onEnable( bool bEnabled ){}
protected:
	void repaint(){}
	void changeCursor( int location ){}
	//////////////

public:
	bool  isEnable(){ return mbEnable; }

protected:

	bool    mbEnable;

	void  reset(int cx,int cy)
	{
		enable(true);

		setRotationAngle( 0.0 );
		setWidth(2*3);
		setHeight(2*3);
		moveTo(cx,cy);

		_this()->repaint();
	}

	void rotate(int cx,int cy)
	{
		if (!mbEnable) return;

		float dx=cx - mCenterPos.x;
		float dy=cy - mCenterPos.y;
		setRotationAngle( Math::ATan2(dx,-dy) );
		_this()->repaint();
	}

	void moveTo(int cx,int cy)
	{
		if (!mbEnable) 
			return;

		mCenterPos = Vec2i(cx, cy);
		_this()->repaint();
	}

	void resize( int location, int PosX , int PosY )
	{
		if (!mbEnable) return;

		Vector2 lPos = worldToLocalPos(Vector2(PosX, PosY));
		int lPosX = Math::CeilToInt(lPos.x + 0.5f);
		int lPosY = Math::CeilToInt(lPos.x + 0.5f);

		switch (location)
		{
		case SIDE_RIGHT:
		case SIDE_LEFT: 
			setWidth(2 *lPosX); break;
		case SIDE_TOP:
		case SIDE_DOWN: 
			setHeight(2 * lPosY); break;
		case CORNER_RT:
		case CORNER_LD:
		case CORNER_LT:
		case CORNER_RD:
			setWidth(2 * lPosX);
			setHeight(2 * lPosY);
			break;
		}
		_this()->repaint();
	}



};

#endif // SelectRectBase_h__