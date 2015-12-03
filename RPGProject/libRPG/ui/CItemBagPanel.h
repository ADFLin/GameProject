#ifndef CItemBagPanel_h__
#define CItemBagPanel_h__

#include "CUICommon.h"
#include "CPlayButton.h"

class TItemStorage;
class CFont;

class CItemBagFrame : public CSlotFrameUI
{
	typedef CSlotFrameUI BaseClass;
public:
	class Button;

	static int const CellSize  = 42;
	static int const BoardSize = 40;
	static int const ItemSize  = 36;
	static int const RowItemNum = 6;

	CItemBagFrame( CActor* actor , Vec2i const& pos );

	void onRender();

	CActor* getActor(){ return m_actor; }
	std::vector< CItemBagFrame::Button* > m_buttons;
	CActor* m_actor;
};


class CItemBagFrame::Button : public CPlayButton
{
	typedef CPlayButton BaseClass;
public:
	Button( unsigned slotPos , Vec2i const& pos , Vec2i const& size  , CWidget* parent );

	void    onClick();
	void    onSetHelpTip( CToolTipUI& helpTip );
	void    onUpdateUI();
	void    onRender();
	void    onInputItem( CPlayButton* button );

	TItemStorage& getItemStorage();
	CActor* getActor();

	//void render( TFont& font );

	unsigned getSlotPos(){ return m_slotPos; }
	void     renderText( CFont& font );

	unsigned m_slotPos;
	int      itemNum;
};

#endif // CItemBagPanel_h__