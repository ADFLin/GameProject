#ifndef CUICommon_h__
#define CUICommon_h__

#include "common.h"
#include "WidgetCommon.h"

#include "CFSprite.h"
#include "CFObject.h"
#include "CFShuFa.h"

class CWidget : public WidgetCoreT< CWidget >
{
public:
	CWidget( Vec2i const& pos , Vec2i const& size , CWidget* parent );
	~CWidget();

	unsigned getID() const { return mSprite->getEntityID();  }
	float    getDepth();
	void     setLocalDepth( float depth );
    void     setOpacity( float val , bool bothChildren = true );
	void     updatePos();
	Sprite*  getSprite(){ return mSprite; }
	void     onLink();

	virtual void  onUpdateUI(){}
	virtual void  onChangePos( Vec2i const& pos , bool bParentMove);
	virtual void  onShow( bool beS );
	virtual void  onRender();
	virtual void  drawText(){}
	void  onChangeChildrenOrder();
	void  onChangeOrder();

	virtual bool onMouseMsg( MouseMsg const& msg );
	void  onMouse( bool beIn ){}
	void  onFocus( bool beF ){}
	virtual void onSetOpactity( float val ){}
	void setupUITextDepth();

	float calcDepthOffset();

	static float const MaxLevelDepth;
	static int const MaxChildNum   = 100;
protected:
	Sprite* mSprite;
};

typedef TWidgetLibrary< CWidget > CUI;


class CButtonUI : public CUI::ButtonT< CButtonUI >
{
public:
	CButtonUI( Vec2i const& pos , Vec2i const& size  , CWidget* parent )
		:CUI::ButtonT< CButtonUI >( pos , size , parent )
	{
		
	}
	virtual void onClick();
	virtual void onChangeState( ButtonState state ){}

};

class CSimpleButton : public CButtonUI
{
public:
	CSimpleButton( char const* dir , char const** picName , Vec2i const& pos , Vec2i const& size , CWidget* parent  );
	virtual void onChangeState( ButtonState state );

protected:
	unsigned mIdButtonRect;
};

class CCloseButton : public CButtonUI
{
public:
	static int const closeSize = 15;
	CCloseButton( Vec2i const& pos , CWidget* parent );

	void onClick()
	{
		getParent()->show( false );
	}
};


enum
{
	PANEL_LB_CORNER = 0,
	PANEL_L_SIDE ,
	PANEL_LT_CORNER ,

	PANEL_B_SIDE ,
	PANEL_CENTER ,
	PANEL_T_SIDE ,

	PANEL_RB_CORNER ,
	PANEL_R_SIDE ,
	PANEL_RT_CORNER ,

	PANEL_POS_NUM ,
};

struct PanelTemplateInfo
{
	int cornerLength;
	int sideLength;
	int sideWidth;

	char const* path;
	char const* texName[ PANEL_POS_NUM ];
};


class CPanelUI : public CUI::PanelT< CPanelUI >
{
	typedef CUI::PanelT< CPanelUI > BaseClass;
public:
	CPanelUI( PanelTemplateInfo& info , 
		      Vec2i const& pos , Vec2i const& size  , 
		      CWidget* parent );

	void      initTemplate( PanelTemplateInfo& info );
	//virtual
	void onShow( bool beS );

protected:
	unsigned    mIdBKSprRect[ PANEL_POS_NUM ];
};



class CToolTipUI : public CPanelUI
{
public:
	CToolTipUI( Vec2i const& pos , Vec2i const& size  , CWidget* parent );
	static PanelTemplateInfo DefultTemplate;

	void onRender();

	void resize( Vec2i const& size );

	void clearString();
	void appendString(char const* str );

	void fitSize();
	void drawText();
	int maxLineFontNum;
	int lineNum;
	String  m_text;
};


class CFrameUI : public CPanelUI
{
public:
	CFrameUI( Vec2i const& pos , Vec2i const& size  , CWidget* parent );

	static PanelTemplateInfo DefultTemplate;
	bool  onMouseMsg( MouseMsg const& msg );

protected:
	CCloseButton* m_closeButton;
};

class CSlotFrameUI : public CFrameUI
{
	typedef CFrameUI BassClass;
public:
	CSlotFrameUI( Vec2i const& pos , Vec2i const& size , CWidget* parent );
	~CSlotFrameUI();
	//virtual
	void onShow( bool beS );
protected:
	Sprite*  mSlotBKSpr;
};

#endif // CUICommon_h__