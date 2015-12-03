#ifndef CFastPlayBar_h__
#define CFastPlayBar_h__

#include "CUICommon.h"
#include "CPlayButton.h"

class CFastPlayBar;

class CFastPlayBar : public CWidget
{
	typedef CWidget BaseClass;
public:
	class Button;

	static int const CellSize  = 42;
	static int const BoardSize = 40;
	static int const ItemSize  = 36;

	static Vec2i const Size;

	static int const FPButtonNum = 10;
	CFastPlayBar( Vec2i const& pos , CWidget* parent );

	virtual void onRender();

	void setButtonFun( int index , CActor* actor , char const* name );
	void playButton( int index );

	Button* getButton( int index ) { return button[index]; }
	Button* button[ FPButtonNum ];
};



class CFastPlayBar::Button : public CPlayButton
{
	typedef CPlayButton BaseClass;
public:
	static int const FPButtonSize = 15;
	Button( int index , Vec2i const& pos , Vec2i const& size , CFastPlayBar* parent );

	void onMouse( bool beIn );

	void onInputItem( CPlayButton* button );
	void onUpdateUI();
	virtual void onRender();

protected:
	int  m_index;
};


#endif // CFastPlayBar_h__
