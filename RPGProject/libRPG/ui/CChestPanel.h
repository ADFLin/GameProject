#ifndef CChestPanel_h__
#define CChestPanel_h__

#include "CUICommon.h"
#include "CPlayButton.h"

class TItemStorage;

class CChestPanel;
class TChestTrigger;

class CChestPanel : public CSlotFrameUI
{
	typedef CSlotFrameUI BaseClass;
public:
	class Button;

	static int const CellSize  = 42;
	static int const BoardSize = 40;
	static int const ItemSize  = 36;
	static int const RowItemNum = 4;
	static int const MaxItemNum = 16;

	CChestPanel( CActor* actor , Vec2i const& pos );

	void setItemStorage( TItemStorage* items );
	bool takeItem( int slotPos );

	std::vector< Button* > m_buttons;

	TItemBase*  getItem( int pos );


	CActor*  m_actor;
	TItemStorage* m_items;
};


class CChestPanel::Button : public CPlayButton
{
	typedef CPlayButton BaseClass;
public:
	Button( unsigned slotPos , Vec2i const& pos , Vec2i const& size  , CChestPanel* parent );

	void onClick();

	void onSetHelpTip( CToolTipUI& helpTip );

	void onUpdateUI(){}
	void onRender(){}
	void onInputItem( CPlayButton* button ){}

	unsigned getSlotPos(){ return m_slotPos; }

	unsigned m_slotPos;
	int      itemNum;
};

#endif // CChestPanel_h__