#ifndef Graphics2DBase_h__
#define Graphics2DBase_h__

#include "Math/TVector2.h"
#include "Core/Color.h"

typedef TVector2< int >   Vec2i;

enum class EVerticalAlign
{
	Top,
	Bottom,
	Center,
	Fill,
};


enum class EHorizontalAlign
{
	Left,
	Right,
	Center,
	Fill,
};

#endif // Graphics2DBase_h__
