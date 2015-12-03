#ifndef ConsoleFrame_h__
#define ConsoleFrame_h__

#include "GameWidget.h"

class ConsoleFrame : public GFrame
{
	typedef GFrame BaseClass;

	ConsoleFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent )
		:BaseClass( id , pos , size , parent )
	{
		mComText = new GTextCtrl( UI_COM_TEXT , Vec2i( 3 , size.y - 3 ) , size.x - 6 , this );
	}

	enum 
	{
		UI_COM_TEXT = BaseClass::NEXT_UI_ID ,
	};

	GTextCtrl* mComText;
	std::vector< std::string > mString;
};

#endif // ConsoleFrame_h__
