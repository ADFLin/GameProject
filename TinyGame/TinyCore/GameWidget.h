#ifndef GameWidget_h__
#define GameWidget_h__

#include "WidgetCore.h"
#include "WidgetCommon.h"

#include "DrawEngine.h"
#include "Singleton.h"
#include "TaskBase.h"
#include "GameControl.h"
#include "Tween.h"
#include <functional>

class DrawEngine;
class StageManager;
class GameController;
struct UserProfile;

typedef std::string String;

enum
{
	EVT_BUTTON_CLICK    ,
	EVT_TEXTCTRL_CHANGE   ,
	EVT_TEXTCTRL_ENTER  ,
	EVT_CHOICE_SELECT   ,
	EVT_LISTCTRL_DCLICK ,
	EVT_LISTCTRL_SELECT ,
	EVT_SLIDER_CHANGE ,
	EVT_CHECK_BOX_CHANGE ,
	EVT_DIALOG_CLOSE,

	EVT_ENTER_UI ,
	EVT_EXIT_UI  ,

	EVT_BOX_YES ,
	EVT_BOX_NO  ,
	EVT_BOX_OK  ,

	EVT_FRAME_FOCUS ,

	EVT_EXTERN_EVENT_ID ,
};

enum
{
	UI_ANY = -1,
	UI_NO  = 0,
	UI_YES ,
	UI_OK  ,
	UI_NEXT_GLOBAL_ID ,

	UI_STAGE_ID           =  400 ,
	UI_GAME_STAGE_MODE_ID =  800 ,
	UI_SUB_STAGE_ID       = 1200 ,
	UI_GAME_ID            = 1600 ,
	UI_LEVEL_MODE_ID      = 2000 ,
	UI_WIDGET_ID          = 2400 ,
	UI_EDITOR_ID          = 2800 ,
};


class RenderCallBack ;
class UIMotionTask;
class WidgetRenderer;
class GWidget;

typedef std::function< bool (int event  , GWidget* ) > WidgetEventDelegate;

class  GWidget : public WidgetCoreT< GWidget >
{
public:
	enum
	{
		NEXT_UI_ID = UI_WIDGET_ID ,
	};
	TINY_API GWidget( Vec2i const& pos , Vec2i const& size , GWidget* parent );
	TINY_API ~GWidget();
	int  getID(){ return mID; }

	virtual bool onChildEvent( int event , int id , GWidget* ui ){  return true; }

	WidgetEventDelegate onEvent;
	TINY_API void  setRenderCallback( RenderCallBack* cb );

	intptr_t getUserData(){ return userData; }
	void     setUserData( intptr_t data ) { userData = data; }
	void     setFontType( int fontType ){ mFontType = fontType; }
	TINY_API void  setHotkey( ControlAction key );

	virtual void onMouse( bool beIn ){  /*sendEvent( ( beIn ) ? EVT_ENTER_UI : EVT_EXIT_UI );*/  }
	virtual void onRender(){}
	virtual void onUpdateUI(){}
	virtual bool onKeyMsg( unsigned key , bool isDown ){ return true; }
	virtual void onHotkey( unsigned key ){}
	virtual void onFocus( bool beF ){}

	virtual void  updateFrame( int frame ){}



	void onPrevRender(){}
	void onPostRenderChildren(){}
	TINY_API void onPostRender();
	TINY_API bool doClipTest();
	static TINY_API WidgetRenderer& getRenderer();

	void setColorKey( Color3ub const& color ){  useColorKey = true; mColorKey = color;  }
	void setColor( int color ){  useColorKey = false; mColor = color;  }
	TINY_API GWidget*  findChild( int id , GWidget* start = NULL );
	template< class T >
	T* findChildT(int id, GWidget* start = nullptr)
	{
		return GUI::CastFast<T>(findChild(id, start));
	}

	void doModal(){  getManager()->startModal( this );  }

	template< class T >
	T* cast()
	{ 
		return GUI::CastFast<T>( this );
	}



protected:

	TINY_API void sendEvent( int eventID );
	TINY_API void removeMotionTask();


	bool      useHotKey;
	intptr_t  userData;

	bool      useColorKey;
	int       mColor;
	Color3ub mColorKey;

	RenderCallBack* callback;
	friend  class UIMotionTask;
	TaskBase* motionTask;
	int       mFontType;
	int       mID;
};

class WidgetPos
{
public:
	typedef GWidget DataType;
	typedef Vector2   ValueType;
	void operator()( DataType& data , ValueType const& value ){ data.setPos( value ); }
};

class WidgetSize
{
public:
	typedef GWidget DataType;
	typedef Vector2   ValueType;
	void operator()( DataType& data , ValueType const& value ){ data.setSize( value ); }
};

typedef TWidgetLibrary< GWidget > GUI;
typedef GUI::Manager         UIManager;


class RenderCallBack 
{
public:
	virtual void postRender( GWidget* ui ) = 0;
	virtual void release(){}

	template < class T , class MemFun >
	static RenderCallBack* Create( T* ptr , MemFun fun );

	template< class Fun >
	static RenderCallBack* Create(Fun fun);

};

enum
{
	GMB_OK ,
	GMB_YESNO ,
	GMB_NONE ,
};


class UIMotionTask;

namespace Private{

	template < class T , class MemFun >
	class TMemberRenderCallBack : public RenderCallBack
	{
		MemFun mFun;
		T*     mPtr;
	public:
		TMemberRenderCallBack( T* ptr , MemFun fun )
			:mPtr(ptr), mFun( fun ){}
		virtual void postRender( GWidget* ui ){  (mPtr->*mFun)( ui );  }
		virtual void release(){ delete this; }
	};

	template< class Fun >
	class TFunctionRenderCallBack : public RenderCallBack
	{
	public:
		TFunctionRenderCallBack( Fun fun ):fun(fun){}
		Fun fun;
		virtual void postRender(GWidget* ui) { fun(ui); }
		virtual void release() { delete this; }
	};
}

template < class T , class MemFun >
RenderCallBack* RenderCallBack::Create( T* ptr , MemFun fun )
{
	return new Private::TMemberRenderCallBack< T , MemFun >( ptr , fun );
}

template < class Fun >
RenderCallBack* RenderCallBack::Create(Fun fun)
{
	return new Private::TFunctionRenderCallBack< Fun >(fun);
}


class  GButtonBase : public GUI::ButtonT< GButtonBase >
{
	typedef GUI::ButtonT< GButtonBase > BaseClass;
public:
	TINY_API GButtonBase( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent )
		:BaseClass( pos , size , parent )
	{
		mID = id;
	}
	virtual void onHotkey( unsigned key ){ onClick();  }
	virtual void onClick(){  sendEvent( EVT_BUTTON_CLICK );  }
};

class  GButton : public GButtonBase
{
	typedef GButtonBase BaseClass;
public:
	TINY_API GButton( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );

	void setTitle( char const* str ){ mTitle = str; }
	virtual void onMouse( bool beInside );
	TINY_API void onRender();
	String mTitle;
};

class GCheckBox : public GButtonBase
{
	typedef GButtonBase BaseClass;
public:
	TINY_API GCheckBox( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );
	virtual void onHotkey( unsigned key ){ onClick();  }
	TINY_API virtual void onClick();
	TINY_API virtual void onMouse(bool beInside) override;
	TINY_API void onRender();
	
	void setTitle( char const* str ){ mTitle = str; }
	bool bChecked;

protected:

	TINY_API void updateMotionState( bool bInside );
	String mTitle;
};

class  GPanel : public GUI::PanelT< GPanel >
{
	typedef GUI::PanelT< GPanel > BaseClass;
public:
	enum RenderType
	{
		eRoundRectType ,
		eRectType ,
	};

	TINY_API GPanel( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );
	TINY_API ~GPanel();

	TINY_API void onRender();
	TINY_API bool onMouseMsg(MouseMsg const& msg);
	void setAlpha( float alpha ){  mAlpha = alpha; }
	void setRenderType( RenderType type ){ mRenderType = type; }

private:
	RenderType  mRenderType;
	float       mAlpha;
	Color3ub   mColor;
};

class  GFrame : public GPanel
{
	typedef GPanel BaseClass;
public:
	TINY_API GFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );
protected:
	TINY_API bool onMouseMsg( MouseMsg const& msg );
	virtual void onFocus(bool beF) { if( beF ) sendEvent(EVT_FRAME_FOCUS); }
};


class  GNoteBook : public GUI::NoteBookT< GNoteBook >
{
public:
	TINY_API GNoteBook( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );

	TINY_API void doRenderButton( PageButton* button );
	TINY_API void doRenderPage( Page* page );
	TINY_API void doRenderBackground();

	Vec2i getButtonSize(){ return Vec2i( 100 , 20 ); }
	Vec2i getButtonOffset(){ return Vec2i( getButtonSize().x + 5 , 0 ); }
	Vec2i getPagePos(){ return Vec2i( 0 , getButtonSize().y + 2 ); }
	Vec2i getBaseButtonPos(){ return Vec2i( 2 , 2 ); }
};

class  GSlider : public GUI::SliderT< GSlider >
{
	typedef GUI::SliderT< GSlider > BaseClass;
public:
	TINY_API GSlider( int id , Vec2i const& pos , int length , bool beH , GWidget* parent );
	static Vec2i TipSize;

	void onScrollChange( int value ){ sendEvent( EVT_SLIDER_CHANGE ); }
	TINY_API void showValue();

	void renderValue( GWidget* widget );

	virtual void onRender();
};

class  GMsgBox : public GPanel
{
public:
	static Vec2i const BoxSize;

	TINY_API GMsgBox( int _id , Vec2i const& size  , GWidget* parent ,unsigned flag = GMB_YESNO);

	void setTitle( char const* str ){ mTitle = str; }
	bool onChildEvent( int event , int id , GWidget* ui );
	TINY_API void onRender();

	String    mTitle;
};


class  GTextCtrl : public GUI::TextCtrlT< GTextCtrl >
{
	typedef GUI::TextCtrlT< GTextCtrl > BaseClass;
public:
	static const int UI_Height = 20;
	TINY_API GTextCtrl( int id , Vec2i const& pos , int length , GWidget* parent );

	void onEditText(){ sendEvent( EVT_TEXTCTRL_CHANGE ); }
	void onPressEnter(){ sendEvent( EVT_TEXTCTRL_ENTER ); }
	TINY_API void onRender();
};

class GText : public GUI::Widget
{
	typedef GUI::Widget BaseClass;
public:
	TINY_API GText( Vec2i const& pos, Vec2i const& size, GWidget* parent);
	TINY_API void onRender();

	GText& setText(char const* pText) { mText = pText; return *this; }
	String mText;
};

class  GChoice : public GUI::ChoiceT< GChoice >
{
	typedef GUI::ChoiceT< GChoice > BaseClass;

public:
	TINY_API GChoice( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );

	TINY_API void onRender();

	void onItemSelect( unsigned select ){  sendEvent( EVT_CHOICE_SELECT );  }
	void doRenderItem( Vec2i const& pos , Item& item , bool beLight );
	void doRenderMenuBG( Menu* menu );
	int  getMenuItemHeight(){ return 20; }

};

class  GListCtrl : public GUI::ListCtrlT< GListCtrl >
{
	typedef GUI::ListCtrlT< GListCtrl > BaseClass;
public:
	TINY_API GListCtrl( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );

	void onItemSelect( unsigned pos ){ ensureVisible(pos); sendEvent( EVT_LISTCTRL_SELECT ); }
	void onItemLDClick( unsigned pos ){ sendEvent( EVT_LISTCTRL_DCLICK ); }
	void doRenderItem( Vec2i const& pos , Item& item , bool beSelected );
	void doRenderBackground( Vec2i const& pos , Vec2i const& size );
	int  getItemHeight(){ return 20; }
};

class GFileListCtrl : public GListCtrl
{
	typedef GListCtrl BaseClass;
public:
	TINY_API GFileListCtrl(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);

	TINY_API String getSelectedFilePath() const;
	TINY_API void deleteSelectdFile();
	TINY_API void setDir(char const* dir);
	TINY_API void refreshFiles();

	String mSubFileName;
	String mCurDir;

};

template< class Fun >
class WidgetMotionTask : public TaskBase
{
public:
	WidgetMotionTask( GWidget* w , Vec2i const& f, Vec2i const& e , long t , long d )
		:widget( w ),from( f ),to( e ),time( t ),delay( d ){ bFinish = false; }

	void onStart()
	{
		::Global::GUI().addMotion< Fun >( widget , from , to , time , delay , std::tr1::bind( WidgetMotionTask< Fun >::onFinish , this ) ); 
	}
	bool onUpdate( long time ){ return false; }
	void onEnd( bool beComplete ){ if ( !beComplete ) widget->setPos( to ); }

	void onFinish(){ bFinish = true; }

	bool      bFinish;
	GWidget*  widget;
	Vec2i     from;
	Vec2i     to;
	long      time;
	long      delay;
};

enum WidgetMotionType
{
	WMT_LINE_MOTION ,
	WMT_SHAKE_MOTION ,
};

class UIMotionTask : public LifeTimeTask
{
public:
	typedef void (UIMotionTask ::*MotionFun)();

	TINY_API UIMotionTask( GWidget* _ui , Vec2i const& _ePos , long time , WidgetMotionType type );
	void   release();
	
	GWidget*  ui;
	Vec2i     sPos;
	Vec2i     ePos;
	long      tParam;
	WidgetMotionType mType;
	
	void onStart();
	bool onUpdate( long time );
	void onEnd( bool beComplete );

	void  LineMotion();
	void  ShakeMotion();
};

class WidgetRenderer
{
public:
	TINY_API void  drawButton( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , int color , bool beEnable = true );
	TINY_API void  drawButton2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , int color , bool beEnable = true );
	TINY_API void  drawPanel( IGraphics2D& g ,Vec2i const& pos , Vec2i const& size , Color3ub const& color , float alpha );
	TINY_API void  drawPanel( IGraphics2D& g ,Vec2i const& pos , Vec2i const& size , Color3ub const& color );
	TINY_API void  drawPanel2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , Color3ub const& color );
	TINY_API void  drawSilder( IGraphics2D& g ,Vec2i const& pos , Vec2i const& size , Vec2i const& tipPos , Vec2i const& tipSize );
};

#endif // GameWidget_h__