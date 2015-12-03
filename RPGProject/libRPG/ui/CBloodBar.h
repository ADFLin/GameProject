#ifndef CBloodBar_h__
#define CBloodBar_h__

#include "CUICommon.h"


class Thinkable;
struct ThinkContent;

class CBloodBar
{
public:
	CBloodBar( Thinkable* thinkObj , CFObject* obj , float maxValue , Vec3D const& pos , Vec2i const& size , float const* color );
	~CBloodBar();

	float getLife() const { return mCurValue; }
	float getMaxVal() const { return mMaxValue; }
	void  setMaxVal(float val);
	void  setLife( float val , bool isFade = true );

protected:
	//virtual
	void  doSetLife( float val );
	void  changeLifeThink( ThinkContent& content );

	float mLength;
	float mMaxValue;
	float mCurValue;
	float mNewVal;
	float mDeltaValue;

	CFObject*     mHostObj;
	ThinkContent* mContent;

	CFly::VertexBuffer* posBuffer;
	unsigned posOffset;
	CFly::VertexBuffer* texBuffer;
	unsigned texOffset;

};


#endif // CBloodBar_h__
