#ifndef CStoreFrame_h__
#define CStoreFrame_h__

#include "CUICommon.h"
#include "CPlayButton.h"

class TItemStorage;
class CActor;
class CNPCBase;

class CStoreFrame;
CStoreFrame* getStorePanel();

class CStoreButton : public CPlayButton
{
public:
	CStoreButton( Vec2i const& pos , Vec2i const& size  , CWidget* parent );

	virtual void  onClick();
	//virtual 
	void onSetHelpTip( CToolTipUI& helpTip );
	virtual void  onInputItem( CPlayButton* button ){}

	void     setSlotPos( unsigned pos ){ m_slotPos = pos; }

	unsigned m_slotPos;
	int      itemNum;
};

enum SellType
{
	SELL_ITEMS ,
	SELL_SKILL ,
};

class CStoreFrame : public CSlotFrameUI
{
	typedef CSlotFrameUI BaseClass;
public:

	static int const CellSize  = 42;
	static int const BoardSize = 40;
	static int const ItemSize  = 36;
	static int const RowItemNum = 2;
	static int const PageShowNum = 10;
	static int const ItemStrLen  = 100;

	CStoreFrame( Vec2i const& pos );
	~CStoreFrame();
	void onRender();

	void onShow( bool beS );

	bool buyItem( int pos );
	void startTrade( SellType type , CNPCBase* seller , 
		             CActor* buyer );

	void pageUp( TEvent const& event );
	void pageDown( TEvent const& event );
	void setHelpTip( int pos , CToolTipUI& helpTip);
	
	
	void  refreshGoods( int page );
	CStoreButton* m_buttons[ PageShowNum ];

	int       m_totalPage;
	int       m_page;
	SellType  m_type;
	CActor*   m_buyer;
	CNPCBase* m_seller;

};


#endif // CStoreFrame_h__
