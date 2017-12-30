#ifndef OptionNoteBook_h__
#define OptionNoteBook_h__

#include "GameWidget.h"
#include "GameControl.h"

class  KeyButton : public GButton
{
public:
	static Vec2i const UI_Size;
	KeyButton( int id , Vec2i const& pos , ControlAction action , GWidget* parent );

	virtual bool onKeyMsg( unsigned key , bool isDown );
	virtual void onClick();
	virtual void onMouse( bool beInside );

	void  setCurKey( unsigned key );
	void  cancelInput();

	ControlAction getAction(){ return mAction ; }

	unsigned mCurKey;
	ControlAction mAction;
	
	static KeyButton*     sInputButton;
	static GameController* sController;
};


class  OptionNoteBook : public GNoteBook
{
public:

	enum
	{
		UI_SET_KEY = UI_WIDGET_ID ,
	};

	static Vec2i UI_Size;

	OptionNoteBook( int _id , Vec2i const& pos  , GWidget* parent  )
		:GNoteBook( _id , pos , UI_Size , parent )
	{

	}
	void init( GameController& controller );
private:
	KeyButton* createKeyButton( Vec2i const& pos , ControlAction action , GWidget* parent );
	void renderControl( GWidget* ui );
	void renderKeyTitle( GWidget* ui );
	void renderUser( GWidget* ui );

};
#endif // OptionNoteBook_h__