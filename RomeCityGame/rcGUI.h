#ifndef rcGUI_h__
#define rcGUI_h__

#include "rcBase.h"
#include "WidgetCommon.h"

class rcControl;

enum
{
	EVT_UI_BTN_CLICK ,
};

struct rcWidgetInfo 
{
	int        id;
	unsigned   group;
	unsigned   texBase;
	Vec2i     pos;
	Vec2i     size;
};

class rcWidget;
typedef TWidgetLibrary< rcWidget > rcGUI;
typedef fastdelegate::FastDelegate< void ( rcWidget* , int , void* ) > FuncUIEvent;

class rcWidget : public WidgetCoreT< rcWidget >
{
public:
	rcWidget( Vec2i const& pos , Vec2i const& size , rcWidget* parent )
		:WidgetCoreT< rcWidget >( pos , size , parent ){}

	void initData( int id )
	{
		mID      = id;
	}
	int getID(){ return mID; }

	template< class T , class TFunc >
	void setEventFunc( T* obj , TFunc func ){  mFuncEvt.bind( obj , func );  }

	virtual void onRender(){}
	virtual void onUpdate(){}

	static Vec2i calcUISize( unsigned texID , int yNum = 1 );

	virtual bool onChildEvent( rcWidget* widget , int evtID , void* data )
	{
		return true;
	}

protected:
	rcWidgetInfo const& getWidgetInfo();
	void procEvent( int evtID , void* data );
	FuncUIEvent mFuncEvt;
	int         mID;
};

typedef TWidgetLibrary< rcWidget > rcGUI;

class rcButton : public rcGUI::ButtonT< rcButton >
{
	typedef rcGUI::ButtonT< rcButton > BaseClass;
public:
	rcButton( int id , Vec2i const& pos , Vec2i const& size , rcWidget* parent )
		:BaseClass( pos , size , parent )
	{
		initData( id );
	}

	void onClick(){  procEvent( EVT_UI_BTN_CLICK , NULL );  }
};


class rcButtonTex3 : public rcButton
{
	typedef rcButton BaseClass;
public:
	rcButtonTex3( int id , unsigned texID , Vec2i const& pos , rcWidget* parent )
		:BaseClass( id , pos , calcUISize( texID , 3 ) , parent ){}

	virtual void onRender();
};

class rcButtonTex4 : public rcButton
{
	typedef rcButton BaseClass;
public:
	rcButtonTex4( int id , unsigned texID , Vec2i const& pos , rcWidget* parent )
		:BaseClass( id , pos , calcUISize( texID , 4 ) , parent ){}

	virtual void onRender();
};


class rcPanel : public rcGUI::PanelT< rcPanel >
{
	typedef rcGUI::PanelT< rcPanel > BaseClass;
public:
	rcPanel( int id , unsigned texID , Vec2i const& pos , rcWidget* parent )
		:BaseClass( pos , calcUISize( texID ) , parent ){ initData( id ); }

	virtual void onRender();
};

class rcControlPanel : public rcPanel
{


	unsigned texFG;
};

class rcSimpleControlPanel : public rcPanel
{


	unsigned texFG;
};


class rcGUISystem : public SingletonT< rcGUISystem >
{
public:
	rcGUI::Manager& getManager(){ return mManager; }
	rcWidget*   loadUI( int uiID , rcWidget* parent = NULL );

	void update( long time ){ mManager.update(); }
	void render()           { mManager.render(); }
	bool procMouse( MouseMsg const& msg )       { return mManager.procMouseMsg( msg ).isHandled() == false; }
	MsgReply procKey(  KeyMsg const& msg ){ return mManager.procKeyMsg(msg); }
	MsgReply procChar( unsigned key )             { return mManager.procCharMsg( key ); }

	void procUIEvent( rcWidget* widget , int evtID , void* data );
	rcControl*    getControl(){ return mControl; }
	void          setControl( rcControl* control ){ mControl = control; }

	rcGUI::Manager mManager;
	rcControl*     mControl;
};

#endif // rcGUI_h__