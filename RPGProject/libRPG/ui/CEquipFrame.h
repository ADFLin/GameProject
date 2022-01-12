#ifndef CEquipFrame_h__
#define CEquipFrame_h__

#include "CUICommon.h"

#include "CPlayButton.h"
#include "TItemSystem.h"

class CActor;
class TEquipTable;
enum  EquipSlot;
class IRenderEntity;

class CModelShowPanel : public CUI::PanelT< CModelShowPanel >
{
	typedef CUI::PanelT< CModelShowPanel > BaseClass;
public:
	CModelShowPanel( IRenderEntity* comp , Vec2i const& pos , Vec2i const& size , CWidget* parent );
	~CModelShowPanel();

	void onRender();
	MsgReply onMouseMsg( MouseMsg const& msg );

	IRenderEntity*  mModel;
	CFCamera*       mCamera;
	CFly::Viewport* mViewport;
};

class CEquipFrame : public CSlotFrameUI
{
	typedef CSlotFrameUI BaseClass;
public:
	class Button;
	static int const CellSize  = 42;
	static int const BoardSize = 40;
	static int const ItemSize  = 36;
	static Vec2i const Size;

	CEquipFrame( CActor* actor , Vec2i const& pos );
	CActor* getActor(){ return m_actor; }
	void    onRender();

	void    drawText();

	Button*       m_equipButton[EQS_EQUIP_TABLE_SLOT_NUM];
	TEquipTable*  m_equipTable;
	CActor*       m_actor;
};

class CEquipFrame::Button : public CPlayButton
{
public:
	static int const FPButtonSize = 15;
	Button( EquipSlot slot ,Vec2i const& pos , Vec2i const& size , CWidget* parent );
	CActor* getActor();
	void    onClick();
	void    onUpdateUI();
	void    onInputItem( CPlayButton* button );

	EquipSlot slot;
};


#endif // CEquipFrame_h__