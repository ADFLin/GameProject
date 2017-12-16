#ifndef GUISystem_h__
#define GUISystem_h__

#include "Base.h"
#include "WidgetCore.h"
#include "WidgetCommon.h"

#include "Singleton.h"


class Texture;
class IText;

enum 
{
	UI_ANY = -1 ,
	UI_STAGE_ID  = 100  ,
	UI_EDIT_ID   = 1000 ,
	UI_WIDGET_ID = 2000,
};

enum
{
	EVT_BUTTON_CLICK    ,
	EVT_TEXTCTRL_CHANGE   ,
	EVT_TEXTCTRL_ENTER  ,
	EVT_CHOICE_SELECT   ,
	EVT_LISTCTRL_DCLICK ,
	EVT_LISTCTRL_SELECT ,
	EVT_SLIDER_CHANGE ,

	EVT_ENTER_UI ,
	EVT_EXIT_UI  ,

	EVT_EXTERN_EVENT_ID ,
};


class  QWidget : public WidgetCoreT< QWidget >
{
public:
	QWidget( Vec2i const& pos , Vec2i const& size , QWidget* parent );
	~QWidget();

	int  getID(){ return mId; }

	virtual bool onChildEvent( int event , int id , QWidget* ui ){  return true; }


	void* getUserData(){ return mUserData; }
	void  setUserData( void* data ) { mUserData = data; }


	virtual void onMouse( bool beIn ){  /*sendEvent( ( beIn ) ? EVT_ENTER_UI : EVT_EXIT_UI );*/  }
	virtual void onRender(){}
	virtual void onUpdateUI(){}
	virtual bool onKeyMsg( unsigned key , bool beDown ){ return true; }
	virtual void onHotkey( unsigned key ){}
	virtual void onFocus( bool beF ){}

	virtual void  updateFrame( int frame ){}

	//Use for Prop Edit
	virtual void  inputData(){}
	virtual void  outputData(){}

	void doRenderAll();
	

	virtual void  onRenderSiblingsEnd(){}
	//bool doClipTest();

	QWidget*  findChild( int id , QWidget* start = NULL );

	void doModal(){  getManager()->startModal( this );  }

protected:

	void sendEvent( int eventID );

	bool   mClipEnable;
	void*  mUserData;
	int    mId;
};

typedef TWidgetLibrary< QWidget > GUI;
typedef GUI::Manager  WidgetManager;

class GUISystem : public SingletonT< GUISystem >
{
public:

	void     render();
	void     sendMessage( int event , int id , QWidget* sender );
	QWidget* findTopWidget( int id , QWidget* start = NULL );
	void     addWidget( QWidget* widget );
	void     cleanupWidget();


	WidgetManager mManager;
};


class  QPanel : public GUI::PanelT< QPanel >
{
	typedef GUI::PanelT< QPanel > BaseClass;
public:
	QPanel( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent );
protected:
	void onRender();
};

class  QButtonBase : public GUI::ButtonT< QButtonBase >
{
	typedef GUI::ButtonT< QButtonBase > BaseClass;
public:
	QButtonBase( int id , Vec2i const& pos , Vec2i const& size  , QWidget* parent )
		:BaseClass( pos , size , parent )
	{
		mId = id;
	}
	virtual void onClick(){  sendEvent( EVT_BUTTON_CLICK );  }
};

class QTextButton : public QButtonBase
{
	typedef QButtonBase BaseClass;
public:
	QTextButton( int id , Vec2i const& pos , Vec2i const& size  , QWidget* parent );
	~QTextButton();
	void   onRender();
	FObjectPtr< IText > text;
};

class QImageButton : public QButtonBase
{
	typedef QButtonBase BaseClass;
public:
	QImageButton( int id , Vec2i const& pos , Vec2i const& size  , QWidget* parent );
	~QImageButton();

	void onRender();
	void setHelpText( char const* str );
	void onRenderSiblingsEnd();
	Texture* texImag;
	IText*   mHelpText;
};

class  QFrame : public GUI::PanelT< QFrame >
{
	typedef GUI::PanelT< QFrame > BaseClass;
public:
	QFrame( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent );
	~QFrame();
	static int const TopSideHeight = 20;
	void setTile( char const* name );

protected:
	bool onMouseMsg( MouseMsg const& msg );
	void onRender();
	

	bool   mbMiniSize;
	Vec2i  mPrevSize;
	IText* mTile;
};


class QTextCtrl : public GUI::TextCtrlT< QTextCtrl >
{
	typedef GUI::TextCtrlT< QTextCtrl > BaseClass;
public:
	QTextCtrl( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent );
	~QTextCtrl();

	void setFontSize( unsigned size );

	void onEditText();
	void onModifyValue();
	void onPressEnter(){ sendEvent( EVT_TEXTCTRL_ENTER ); }
	virtual void onFocus( bool beF );
	void onRender();

	IText* text;
};

class QChoice : public GUI::ChoiceT< QChoice >
{
	typedef GUI::ChoiceT< QChoice > BaseClass;
public:

	QChoice( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent );

	struct MyData 
	{
		IText* text;
	};

	typedef MyData ExtraData;

	void onRender();

	void onItemSelect( unsigned select ){  sendEvent( EVT_CHOICE_SELECT );  }
	int  getMenuItemHeight(){ return 20; }
	void onAddItem( Item& item );
	void onRemoveItem( Item& item );
	void doRenderItem( Vec2i const& pos , Item& item , bool beLight );
	void doRenderMenuBG( Menu* menu );
};

class QListCtrl : public GUI::ListCtrlT< QListCtrl >
{
	typedef GUI::ListCtrlT< QListCtrl > BaseClass;
public:

	QListCtrl( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent );

	struct MyData 
	{
		IText* text;
	};
	typedef MyData ExtraData;

	void onItemSelect( unsigned pos ){ sendEvent( EVT_LISTCTRL_SELECT ); }
	void onItemLDClick( unsigned pos ){ sendEvent( EVT_LISTCTRL_DCLICK ); }
	void onAddItem( Item& item );
	void onRemoveItem( Item& item );
	void doRenderItem( Vec2i const& pos , Item& item , bool beSelected );
	void doRenderBackground( Vec2i const& pos , Vec2i const& size );
	int  getItemHeight(){ return 20; }

};


#endif // GUISystem_h__
