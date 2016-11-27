#ifndef GameWidget_h__
#define GameWidget_h__

#include "TUICore.h"
#include "TUICommon.h"

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

	EVT_ENTER_UI ,
	EVT_EXIT_UI  ,

	EVT_BOX_YES ,
	EVT_BOX_NO  ,
	EVT_BOX_OK  ,

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
	UI_WEIGET_ID          = 2400 ,
	UI_EDITOR_ID          = 2800 ,
};


class RenderCallBack ;
class UIMotionTask;
class WidgetRenderer;

class  GWidget : public TUICore< GWidget >
{
public:
	enum
	{
		NEXT_UI_ID = UI_WEIGET_ID ,
	};
	GAME_API GWidget( Vec2i const& pos , Vec2i const& size , GWidget* parent );
	GAME_API ~GWidget();
	int  getID(){ return mID; }

	virtual bool onChildEvent( int event , int id , GWidget* ui ){  return true; }

	GAME_API void  setRenderCallback( RenderCallBack* cb );

	intptr_t getUserData(){ return userData; }
	void     setUserData( intptr_t data ) { userData = data; }
	void     setFontType( int fontType ){ mFontType = fontType; }
	GAME_API void  setHotkey( ControlAction key );

	virtual void onMouse( bool beIn ){  /*sendEvent( ( beIn ) ? EVT_ENTER_UI : EVT_EXIT_UI );*/  }
	virtual void onRender(){}
	virtual void onUpdateUI(){}
	virtual bool onKeyMsg( unsigned key , bool isDown ){ return true; }
	virtual void onHotkey( unsigned key ){}
	virtual void onFocus( bool beF ){}

	virtual void  updateFrame( int frame ){}



	void onPrevRender(){}
	void onPostRenderChildren(){}
	GAME_API void onPostRender();
	GAME_API bool doClipTest();
	static GAME_API WidgetRenderer& getRenderer();

	void setColorKey( ColorKey3 const& color ){  useColorKey = true; mColorKey = color;  }
	void setColor( int color ){  useColorKey = false; mColor = color;  }
	GAME_API GWidget*  findChild( int id , GWidget* start = NULL );

	void doModal(){  getManager()->startModal( this );  }

	template< class T >
	T* cast()
	{ 
		assert( dynamic_cast< T* >( this ) );
		return static_cast< T* >( this );
	}

protected:

	GAME_API void sendEvent( int eventID );
	GAME_API void removeMotionTask();

	bool      useHotKey;
	intptr_t  userData;

	bool      useColorKey;
	int       mColor;
	ColorKey3 mColorKey;

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
	typedef Vec2f   ValueType;
	void operator()( DataType& data , ValueType const& value ){ data.setPos( value ); }
};

class WidgetSize
{
public:
	typedef GWidget DataType;
	typedef Vec2f   ValueType;
	void operator()( DataType& data , ValueType const& value ){ data.setSize( value ); }
};

typedef UIPackage< GWidget > GUI;
typedef GUI::Manager         UIManager;


class RenderCallBack 
{
public:
	virtual void postRender( GWidget* ui ) = 0;
	virtual void release(){}

	template < class T , class MemFun >
	static RenderCallBack* create( T* ptr , MemFun fun );

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
	class MemberRenderCallBack : public RenderCallBack
	{
		MemFun mFun;
		T*     mPtr;
	public:
		MemberRenderCallBack( T* ptr , MemFun fun )
			:mPtr(ptr), mFun( fun ){}
		virtual void postRender( GWidget* ui ){  (mPtr->*mFun)( ui );  }
		virtual void release(){ delete this; }
	};
}

template < class T , class MemFun >
RenderCallBack* RenderCallBack::create( T* ptr , MemFun fun )
{
	return new Private::MemberRenderCallBack< T , MemFun >( ptr , fun );
}




#include <string>


class  GButtonBase : public GUI::Button< GButtonBase >
{
	typedef GUI::Button< GButtonBase > BaseClass;
public:
	GAME_API GButtonBase( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent )
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
	GAME_API GButton( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );

	void setTitle( char const* str ){ mTitle = str; }
	virtual void onMouse( bool beInside );
	GAME_API void onRender();
	std::string mTitle;
};

class GCheckBox : public GButtonBase
{
	typedef GButtonBase BaseClass;
public:
	GAME_API GCheckBox( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );
	virtual void onHotkey( unsigned key ){ onClick();  }
	virtual void onClick(){  isCheck = !isCheck; sendEvent( EVT_CHECK_BOX_CHANGE );   }
	GAME_API void onRender();
	void setTitle( char const* str ){ mTitle = str; }
	bool isCheck;
	std::string mTitle;
};

class  GPanel : public GUI::Panel< GPanel >
{
	typedef GUI::Panel< GPanel > BaseClass;
public:
	enum RenderType
	{
		eRoundRectType ,
		eRectType ,
	};

	GAME_API GPanel( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );
	GAME_API ~GPanel();

	GAME_API void onRender();
	void setAlpha( float alpha ){  mAlpha = alpha; }
	void setRenderType( RenderType type ){ mRenderType = type; }

private:
	RenderType  mRenderType;
	float       mAlpha;
	ColorKey3   mColor;
};

class  GFrame : public GPanel
{
	typedef GPanel BaseClass;
public:
	GAME_API GFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );
protected:
	GAME_API bool onMouseMsg( MouseMsg const& msg );
};


class  GNoteBook : public GUI::NoteBook< GNoteBook >
{
public:
	GAME_API GNoteBook( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );

	GAME_API void doRenderButton( PageButton* button );
	GAME_API void doRenderPage( Page* page );
	GAME_API void doRenderBackground();

	Vec2i getButtonSize(){ return Vec2i( 100 , 20 ); }
	Vec2i getButtonOffset(){ return Vec2i( getButtonSize().x + 5 , 0 ); }
	Vec2i getPagePos(){ return Vec2i( 0 , getButtonSize().y + 2 ); }
	Vec2i getBaseButtonPos(){ return Vec2i( 2 , 2 ); }
};

class  GSlider : public GUI::Slider< GSlider >
{
	typedef GUI::Slider< GSlider > BaseClass;
public:
	GAME_API GSlider( int id , Vec2i const& pos , int length , bool beH , GWidget* parent );
	static Vec2i TipSize;

	void onScrollChange( int value ){ sendEvent( EVT_SLIDER_CHANGE ); }
	GAME_API void showValue();

	void renderValue( GWidget* widget );

	virtual void onRender();
};

class  GMsgBox : public GPanel
{
public:
	static Vec2i const BoxSize;

	GAME_API GMsgBox( int _id , Vec2i const& size  , GWidget* parent ,unsigned flag = GMB_YESNO);

	void setTitle( char const* str ){ mTitle = str; }
	bool onChildEvent( int event , int id , GWidget* ui );
	GAME_API void onRender();

	std::string     mTitle;
};


class  GTextCtrl : public GUI::TextCtrl < GTextCtrl >
{
	typedef GUI::TextCtrl < GTextCtrl > BaseClass;
public:
	static const int UI_Height = 20;
	GAME_API GTextCtrl( int id , Vec2i const& pos , int length , GWidget* parent );

	void onEditText(){ sendEvent( EVT_TEXTCTRL_CHANGE ); }
	void onPressEnter(){ sendEvent( EVT_TEXTCTRL_ENTER ); }
	GAME_API void onRender();
};

class  GChoice : public GUI::Choice< GChoice >
{
	typedef GUI::Choice< GChoice > BaseClass;

public:
	GAME_API GChoice( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );

	GAME_API void onRender();

	void onItemSelect( unsigned select ){  sendEvent( EVT_CHOICE_SELECT );  }
	void doRenderItem( Vec2i const& pos , Item& item , bool beLight );
	void doRenderMenuBG( Menu* menu );
	int  getMenuItemHeight(){ return 20; }

};

class  GListCtrl : public GUI::ListCtrl< GListCtrl >
{
	typedef GUI::ListCtrl< GListCtrl > BaseClass;
public:
	GAME_API GListCtrl( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );

	void onItemSelect( unsigned pos ){ ensureVisible(pos); sendEvent( EVT_LISTCTRL_SELECT ); }
	void onItemLDClick( unsigned pos ){ sendEvent( EVT_LISTCTRL_DCLICK ); }
	void doRenderItem( Vec2i const& pos , Item& item , bool beSelected );
	void doRenderBackground( Vec2i const& pos , Vec2i const& size );
	int  getItemHeight(){ return 20; }
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

	GAME_API UIMotionTask( GWidget* _ui , Vec2i const& _ePos , long time , WidgetMotionType type );
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
	GAME_API void  drawButton( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , int color , bool beEnable = true );
	GAME_API void  drawButton2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , int color , bool beEnable = true );
	GAME_API void  drawPanel( IGraphics2D& g ,Vec2i const& pos , Vec2i const& size , ColorKey3 const& color , float alpha );
	GAME_API void  drawPanel( IGraphics2D& g ,Vec2i const& pos , Vec2i const& size , ColorKey3 const& color );
	GAME_API void  drawPanel2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ColorKey3 const& color );
	GAME_API void  drawSilder( IGraphics2D& g ,Vec2i const& pos , Vec2i const& size , Vec2i const& tipPos , Vec2i const& tipSize );
};

#endif // GameWidget_h__